//===----------------------------------------------------------------------===//
//
//                                  tinyOS
//                             The Monix Kernel
//
// 	This program is free software: you can redistribute it and/or modify
// 	it under the terms of the GNU General Public License as published by
// 	the Free Software Foundation, either version 3 of the License, or
// 	(at your option) any later version.
//
// 	This program is distributed in the hope that it will be useful,
// 	but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// 	GNU General Public License for more details.
//
// 	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//	Copyright (C) 2023-2025, Harry Moulton <me@h3adsh0tzz.com>
//
//===----------------------------------------------------------------------===//

/**
 * Name:	exception.c
 * Desc:	Kernel Exception Handlers.
*/

#include <tinylibc/stdint.h>
#include <tinylibc/string.h>
#include <tinylibc/stddef.h>

#include <kern/machine/machine_timer.h>
#include <kern/machine/machine-irq.h>

#include <kern/defaults.h>
#include <kern/trace/printk.h>
#include <kern/vm/vm.h>
#include <kern/sched.h>
#include <kern/task.h>
#include <kern/cpu.h>

#include <arch/arch.h>
#include <libkern/panic.h>
#include <libkern/version.h>

/* kernel handler annotations */
#define __KERNEL_FAULT_HANDLER
#define __KERNEL_ABORT_HANDLER

/* Types that will only be used here */
typedef vm_address_t	fault_address_t;
typedef int				fault_type_t;

/**
 * AArch64 First-stage Exception Handlers
*/
void arm64_handler_synchronous (arm64_exception_frame_t *);
void arm64_handler_serror (arm64_exception_frame_t *);
void arm64_handler_fiq (arm64_exception_frame_t *);
void arm64_handler_irq (arm64_exception_frame_t *);

/**
 * Second-stage Exception Handlers
*/
static void handle_breakpoint (arm64_exception_frame_t *);
static void handle_svc (arm64_exception_frame_t *);

/**
 * The Abort inspector determines the type of Abort that has occured, and the
 * handler is then dispatched to deal with the exception. From the First-stage
 * exception handler, we just call handle_abort().
*/
typedef void (*abort_inspector_t)	(fault_status_t *, fault_type_t *, uint32_t);
typedef void (*abort_handler_t)		(arm64_exception_frame_t *, fault_status_t, fault_status_t);

/* Abort Inspectors */
static void inspect_data_abort(fault_status_t *, fault_type_t *, uint32_t);
static void inspect_instruction_abort(fault_status_t *, fault_type_t *, uint32_t);

/* Abort type handlers */
static void handle_data_abort(arm64_exception_frame_t *, fault_address_t, fault_status_t);
static void handle_instruction_abort(arm64_exception_frame_t *, fault_address_t, fault_status_t);
static void handle_prefetch_abort(arm64_exception_frame_t *, fault_address_t, fault_status_t);
static void handle_msr_trap(arm64_exception_frame_t *);

/* General Abort handler */
static void handle_abort(arm64_exception_frame_t *, abort_handler_t, abort_inspector_t);

/* Undefined Instruction handler */
static void handle_undefined_instruction(arm64_exception_frame_t *);

/**
 * Misc Helper Functions
*/
static inline int is_translation_fault(fault_status_t);
static inline int is_address_size_fault(fault_status_t);
static inline int is_permission_fault(fault_status_t);
static inline int is_alignment_fault(fault_status_t);
static inline int is_vm_fault(fault_status_t);

static inline int vm_fault_get_level(fault_status_t);


/*******************************************************************************
 * Misc Helper Functions
*******************************************************************************/

static inline int is_vm_fault(fault_status_t status)
{
	switch (status) {
		case FSC_TRANSLATION_FAULT_L0:
		case FSC_TRANSLATION_FAULT_L1:
		case FSC_TRANSLATION_FAULT_L2:
		case FSC_TRANSLATION_FAULT_L3:
		case FSC_ACCESS_FLAG_FAULT_L1:
		case FSC_ACCESS_FLAG_FAULT_L2:
		case FSC_ACCESS_FLAG_FAULT_L3:
		case FSC_PERMISSION_FAULT_L1:
		case FSC_PERMISSION_FAULT_L2:
		case FSC_PERMISSION_FAULT_L3:
			return 1;
		default:
			return 0;
	}
}

static inline int is_translation_fault(fault_status_t status)
{
	switch (status) {
		case FSC_TRANSLATION_FAULT_L0:
		case FSC_TRANSLATION_FAULT_L1:
		case FSC_TRANSLATION_FAULT_L2:
		case FSC_TRANSLATION_FAULT_L3:
			return 1;
		default:
			return 0;
	}
}

static inline int is_address_size_fault(fault_status_t status)
{
	switch (status) {
		case FSC_ADDRESS_SIZE_FAULT_L0:
		case FSC_ADDRESS_SIZE_FAULT_L1:
		case FSC_ADDRESS_SIZE_FAULT_L2:
		case FSC_ADDRESS_SIZE_FAULT_L3:
			return 1;
		default:
			return 0;
	}
}

static inline int is_permission_fault(fault_status_t status)
{
	switch (status) {
		case FSC_PERMISSION_FAULT_L1:
		case FSC_PERMISSION_FAULT_L2:
		case FSC_PERMISSION_FAULT_L3:
			return 1;
		default:
			return 0;
	}
}

static inline int is_alignment_fault(fault_status_t status)
{
	return (FSC_ALIGNMENT_FAULT == status);
}


/**
 * Name:	vm_fault_get_level
 * Desc:	Return the Translation Level the given fault occured at, or -1 if
 * 			the given fault_status_t is not a Translation fault.
*/
static inline int vm_fault_get_level(fault_status_t status)
{
	switch (status) {
		/* Level 0 */
		case FSC_TRANSLATION_FAULT_L0:
		case FSC_ADDRESS_SIZE_FAULT_L0:
			return 0;

		/* Level 1 */
		case FSC_TRANSLATION_FAULT_L1:
		case FSC_ADDRESS_SIZE_FAULT_L1:
		case FSC_ACCESS_FLAG_FAULT_L1:
		case FSC_PERMISSION_FAULT_L1:
		case FSC_SYNC_EXT_ABORT_TT_L1:
		case FSC_SYNC_PARITY_TT_L1:
			return 1;

		/* Level 2 */
		case FSC_TRANSLATION_FAULT_L2:
		case FSC_ADDRESS_SIZE_FAULT_L2:
		case FSC_ACCESS_FLAG_FAULT_L2:
		case FSC_PERMISSION_FAULT_L2:
		case FSC_SYNC_EXT_ABORT_TT_L2:
		case FSC_SYNC_PARITY_TT_L2:
			return 2;

		/* Level 3 */
		case FSC_TRANSLATION_FAULT_L3:
		case FSC_ADDRESS_SIZE_FAULT_L3:
		case FSC_ACCESS_FLAG_FAULT_L3:
		case FSC_PERMISSION_FAULT_L3:
		case FSC_SYNC_EXT_ABORT_TT_L3:
		case FSC_SYNC_PARITY_TT_L3:
			return 3;

		/* Not a translation fault */
		default:
			return -1;
	}
}


/**
 * Name:	inspect_data_abort
 * Desc:	Determine the fault type. This was borrowed from XNU. Inspect the 
 * 			ISS value to determine the fault_code and fault_status, to determine
 * 			whether the abort was on a read or a write.
*/
static void inspect_data_abort(fault_status_t *fault_code, 
		fault_type_t *fault_type, uint32_t iss)
{
	*fault_code = ISS_DA_FSC(iss);

	if ((iss & ISS_DA_WNR) && (!(iss & ISS_DA_CM) || is_permission_fault(*fault_code))) {
		*fault_type = (VM_PROT_READ | VM_PROT_WRITE);
	} else {
		*fault_type = VM_PROT_READ;
	}
}

/**
 * Name:	inspect_instruction_abort
 * Desc:	Determine the fault type. This was borrowed from XNU. Inspect the
 * 			ISS value to determine the default_code and fault_status for the
 * 			instruction abort.
*/
static void inspect_instruction_abort(fault_status_t *fault_code,
		fault_type_t *fault_type, uint32_t iss)
{
	*fault_code = ISS_IA_FSC(iss);
	*fault_type = (VM_PROT_READ | VM_PROT_EXECUTE);
}


/*******************************************************************************
 * Abort Handlers
*******************************************************************************/

/**
 * TODO: Think of how this will need to change when userland is added. When a
 * userland process triggers an exception we don't want to halt the CPU, only a
 * kernel (kernel_task) exception should do that.
 * 
 * At the moment, i'm leaning towards checking kernel/user land within the
 * "handle_data_abort", rather than the exception handler further down (which is
 * how XNU does it).
 * 
 * The abort handlers shouldn't halt the CPU. Halting the CPU should be reserved
 * for the first-stage exception handlers. 
 * 
*/

__KERNEL_ABORT_HANDLER
void handle_data_abort(arm64_exception_frame_t *frame, 
		fault_address_t fault_address, fault_status_t fault_status)
{
	/**
	 * Panic with a virtual memory Translation Fault, and fetch the level at
	 * which the fault occured.
	*/
	if (is_translation_fault(fault_status)) {
		/** TOOD: There is more handling to do here then just printing the fault type */
		panic_with_thread_state(frame, "Data Abort - Translation Fault Level %d", 
			vm_fault_get_level(fault_status));
		return;
	}

	/**
	 * Panic with a virtual memory Permission Fault, and fetch the level at
	 * which the fault occured.
	*/
	if (is_permission_fault(fault_status)) {
		panic_with_thread_state(frame, "Data Abort - Permissions Fault, Level %d",
			vm_fault_get_level(fault_status));
		return;
	}

	/**
	 * Panic with an Alignment Fault.
	*/
	if (is_alignment_fault(fault_status)) {
		panic_with_thread_state(frame, "Alignment Fault");
		return;
	}

	/**
	 * Panic with Address Size Fault
	*/
	if (is_address_size_fault(fault_status)) {
		panic_with_thread_state(frame, "Data Abort - Address Size Fault, Level %d",
			vm_fault_get_level(fault_status));
		return;
	}

	panic_with_thread_state(frame, "Data Abort - Unknown (0x%lx)",
		fault_status);
}

__KERNEL_ABORT_HANDLER
void handle_instruction_abort(arm64_exception_frame_t *frame,
		fault_address_t fault_address, fault_status_t fault_status)
{
	/**
	 * Panic with Translation Fault.
	*/
	if (is_translation_fault(fault_status)) {
		panic_with_thread_state(frame, "Kernel Instruction Abort - Translation Fault, Level %d",
			vm_fault_get_level(fault_status));
		return;
	}

	panic_with_thread_state(frame, "Kernel Instruction Abort - Unknwon (0x%x)",
		fault_status);
}


/*******************************************************************************
 * Additional Handlers
*******************************************************************************/

void handle_undefined_instruction(arm64_exception_frame_t *frame)
{
	panic_with_thread_state(frame, "Undefined Instruction");
}

/*******************************************************************************
 * Second-stage Exception Handling
*******************************************************************************/

/**
 * Name:	handle_breakpoint
 * Desc:	Handle a Breakpoint Exception.
*/
__KERNEL_FAULT_HANDLER
void handle_breakpoint(arm64_exception_frame_t *frame)
{
	panic("Breakpoint 64");
}

/**
 * Name:	handle_svc
 * Desc:	Handle a Supervisor Call Exception.
*/
__KERNEL_FAULT_HANDLER
void handle_svc(arm64_exception_frame_t *frame)
{
	panic("Supervisor Call (64)");
}

/**
 * Name:	handle_abort
 * Desc:	Handle an Abort exception (Data, Instruction, Prefetch)
*/
__KERNEL_FAULT_HANDLER
void handle_abort(arm64_exception_frame_t *frame, abort_handler_t handler, abort_inspector_t inspect)
{
	fault_address_t		fault_address;
	fault_status_t		fault_code;
	fault_type_t		fault_type;

	/* Inspect the fault, and then call the handler */
	inspect(&fault_code, &fault_type, ESR_ISS(frame->esr));
	handler(frame, fault_address, fault_code);
}

/**
 * Name:	handle_msr_trap
 * Desc:	Handle a trapped MSR, MRS or System instruction.
*/
__KERNEL_ABORT_HANDLER
void handle_msr_trap(arm64_exception_frame_t *frame)
{
	panic(frame->fp, "Trapped MSR, MRS, or System instruction");
}

/*******************************************************************************
 * First-stage Exception Handling
*******************************************************************************/

void arm64_handler_synchronous(arm64_exception_frame_t *frame)
{
	esr_exception_class_t	class;
	//thread_t				thread;

	/* obtain the exception class from the ESR register */
	class = ESR_EC(frame->esr);

	/* handle each of the exception classes */
	switch (class) {
		/* Program Counter Alignment Fault */
		case ESR_EC_PC_ALIGN:
			/* tmp */
			panic_with_thread_state(frame, "PC Alignment Fault");
			cpu_halt ();
			break;

		/* Data Abort (EL0 and EL1) */
		case ESR_EC_DABORT_EL0:
		case ESR_EC_DABORT_EL1:
			handle_abort(frame, (abort_handler_t) handle_data_abort, inspect_data_abort);
			cpu_halt();
			break;

		/* Breakpoint */
		case ESR_EC_BRK_AARCH64:
			handle_breakpoint(frame);
			cpu_halt();
			break;

		/* Supervisor Call */
		case ESR_EC_SVC_64:
			handle_svc(frame);
			break;

		/* MSR Trap */
		case ESR_EC_MSR_TRAP:
			handle_msr_trap(frame);
			cpu_halt();
			break;

		/* Instruction Abort (EL0 and EL1) */
		case ESR_EC_IABORT_EL1:
		case ESR_EC_IABORT_EL0:
			handle_abort(frame, (abort_handler_t) handle_instruction_abort, inspect_instruction_abort);
			cpu_halt();
			break;

		/* Undefined Instruction */
		case ESR_EC_UNCATEGORIZED:
			handle_undefined_instruction(frame);
			cpu_halt();
			break;

		/* Unknown Exception*/
		default:
			kprintf("class: 0x%x\n", class);
			panic_with_thread_state(frame, "Unknown Exception");
			cpu_halt();
			break;
	}
}

void arm64_handler_serror(arm64_exception_frame_t *frame)
{
	panic("arm64_handler_serror\n");
}

void arm64_handler_fiq(arm64_exception_frame_t *frame)
{
	uint32_t intid = sysreg_read(icc_iar1_el1);
	sysreg_write(icc_eoir1_el1, intid);

	kprintf("arm64_handler_fiq: intid: %d\n", intid);
}

int irq_count = 0;

void arm64_handler_irq(arm64_exception_frame_t *frame)
{
	uint32_t intid = sysreg_read(icc_iar1_el1);
	sysreg_write(icc_eoir1_el1, intid);

	machine_irq_disable();

#if DEFAULTS_KERNEL_SCHED_DEBUG_MSG
	kprintf("==== SYSTEM IRQ HANDLER ====\n");
	kprintf ("arm64_handler_irq(%d): intid: %d\n", irq_count, intid);
#endif

	irq_count++;

#if DEFAULTS_KERNEL_SCHED_DEBUG_MSG
	kprintf("==== SYSTEM IRQ HANDLER ====\n");
#endif

	if (intid == 30) {
#if DEFAULTS_KERNEL_SCHED_DEBUG_MSG
		kprintf("machine_timer_reset(%d)\n", MACHINE_TIMER_RESET_VALUE);
#endif
		machine_timer_reset(MACHINE_TIMER_RESET_VALUE);
		__schedule(frame);
	}
}
