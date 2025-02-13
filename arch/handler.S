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
 *	Name:	handler.S
 *	Desc:	AArch64 Exception handler
 */

/*******************************************************************************
 * Helper macros
 ******************************************************************************/

/* create the exception stack frame for an exception using SP0 */
.macro create_exception_frame_sp0
	msr		SPSel, #0				// Switch to SP0
	sub		sp, sp, #400			// Create the exception frame
	stp		x0, x1, [sp, #0]		// Save x0 and x1 to the exception frame
	add		x0, sp, #400			// Calculate the original SP
	str		x0, [sp, #248]			// Save the SP to the exception frame
	str		fp, [sp, #232]			// Save the FP to the exception frame
	mov		x0, sp					// Copy saved state pointer to x0
.endm

/* create the exception stack frame for an exception using SP1 */
.macro create_exception_frame_sp1
	sub		sp, sp, #400		
	stp		x0, x1, [sp, #0]	
	add		x0, sp, #400		
	str		x0, [sp, #240]		
	mov		x0, sp				
.endm

/* save the exception registers to the exception frame */
.macro save_exception_registers
	mrs		x1, FAR_EL1
	str		x1, [x0, #256]
	mrs		x1, ESR_EL1
	str		x1, [x0, #264]
.endm

/* exit the exception handler */
.macro exit_exception
	msr		SPSel, #1
.endm

/*******************************************************************************
 * Exception Handling
 ******************************************************************************/

/**
 * _ExceptionVectorBase
 *
 * Exception vector base. The handlers for each type of exception are placed at
 * offsets from this base as outlined in the architecture reference manual,
 * and this must be aligned on a 12-byte boundary.
 */
	.align 	12
	.globl 	_ExceptionVectorBase
_ExceptionVectorBase:

	/* EL1 SP0 */
L__el1_sp0_synchronous_handler:
	create_exception_frame_sp0
	save_exception_registers
	adr		x1, arm64_handler_synchronous
	b		L__dispatch64

	.align 7
L__el1_sp0_irq_handler:
	create_exception_frame_sp0
	adr		x1, arm64_handler_irq
	b		L__dispatch64

	.align 7
L__el1_sp0_fiq_handler:
	create_exception_frame_sp0
	adr		x1, arm64_handler_fiq
	b		L__dispatch64

	.align 7
L__el1_sp0_serror_handler:
	create_exception_frame_sp0
	adr		x1, arm64_handler_serror
	b		L__dispatch64

	/* EL1 SP1 */
	.align 7
L__el1_sp1_synchronous_handler:
	// todo: check that the SP is still within the exception stack
	create_exception_frame_sp1
	adr		x1, arm64_handler_synchronous
	b		L__dispatch64

	.align 7
L__el1_sp1_irq_handler:
	create_exception_frame_sp1
	adr		x1, arm64_handler_irq
	b		L__dispatch64

	.align 7
L__el1_sp1_fiq_handler:
	create_exception_frame_sp1
	adr		x1, arm64_handler_fiq
	b		L__dispatch64

	.align 7
L__el1_sp1_serror_handler:
	create_exception_frame_sp1
	adr		x1, arm64_handler_serror
	b		L__dispatch64

	/* EL0 64 */
	.align 7
L__el0_64_synchronous_handler:
	b	.

	.align 7
L__el0_64_irq_handler:
	b	.

	.align 7
L__el0_64_fiq_handler:
	b	.

	.align 7
L__el0_64_serror_handler:
	b	.

	/**
	 * TinyOS and Monix do not support AArch32 execution, so the remaining
	 * entries are not important, so fill the rest of the table.
	 */
	.align 12

/**
 * __dispatch64
 *
 * AArch64 exception handler dispatcher. Called from the exception vector table,
 * the exception frame should have been created and placed in x0, and the
 * address of the C handler in x1. __dispatch64 will save the remaining general
 * purpose registers into the exception frame, save the elr_el1 and branch to
 * the C handler. Once this has completed we exit the exception handler, if we
 * were allowed to return from the C handler.
 */
	.align	2
L__dispatch64:

	/* save remaining registers */
	stp		x2, x3, 	[x0, #16 * 1]
	stp		x4, x5, 	[x0, #16 * 2]
	stp		x6, x7, 	[x0, #16 * 3]
	stp		x8, x9, 	[x0, #16 * 4]
	stp		x10, x11, 	[x0, #16 * 5]
	stp		x12, x13, 	[x0, #16 * 6]
	stp		x14, x15, 	[x0, #16 * 7]
	stp		x16, x17, 	[x0, #16 * 8]
	stp		x18, x19, 	[x0, #16 * 9]
	stp		x20, x21, 	[x0, #16 * 10]
	stp		x22, x23, 	[x0, #16 * 11]
	stp		x24, x25, 	[x0, #16 * 12]
	stp		x26, x27, 	[x0, #16 * 13]
	str		x28, 		[x0, #16 * 14]

	/**
	 * save the exception link register (ELR_EL1), frame->elr isn't aligned to
	 * 16-bytes so use the offset and a single store.
	 */
	mrs		x22, ELR_EL1
	str		x22, [x0, #272]

	mov		x28, x0
	blr		x1
	b		L__exception_exit


/**
 * __exception_exit
 *
 * Loads the stack pointer and link register from the exception frame, and
 * restores the general-purpose registers. Switch the stack pointer back to SP0
 * and eret back to where the kernel was before the exception occured.
 */
	.align 2
L__exception_exit:

	mov		x0, x28

	/* load the context stack pointer */
	ldr		x22, [x0, #248]
	msr		SPSR_EL1, x22

	/* load the exception link regsiter */
	ldr		x22, [x0, #272]
	msr		ELR_EL1, x22

	/* load remaining registers */
	ldp		x2, x3, 	[x0, #16 * 1]
	ldp		x4, x5, 	[x0, #16 * 2]
	ldp		x6, x7, 	[x0, #16 * 3]
	ldp		x8, x9, 	[x0, #16 * 4]
	ldp		x10, x11, 	[x0, #16 * 5]
	ldp		x12, x13, 	[x0, #16 * 6]
	ldp		x14, x15, 	[x0, #16 * 7]
	ldp		x16, x17, 	[x0, #16 * 8]
	ldp		x18, x19, 	[x0, #16 * 9]
	ldp		x20, x21, 	[x0, #16 * 10]
	ldp		x22, x23, 	[x0, #16 * 11]
	ldp		x24, x25, 	[x0, #16 * 12]
	ldp		x26, x27, 	[x0, #16 * 13]
	ldp		x28, fp, 	[x0, #16 * 14]
	ldr		lr, 		[x0, #16 * 15]

	/* switch back to SP0 */
	exit_exception
	eret

/*******************************************************************************
 * Context Switching
 ******************************************************************************/

/**
 * __fork64_exec
 *
 * Prepare to switch execution to a new thread. The thread context is passed via
 * x0, and the callee-saved registers and stack pointer are loaded from that
 * context. The scheduler will load the address of __fork64_return into the link
 * register.
 */
	.align	12
	.globl	__fork64_exec
__fork64_exec:

	/* load the register context */
	ldp		x19, x20, [x0, #16 * 0]
	ldp		x21, x22, [x0, #16 * 1]
	ldp		x23, x24, [x0, #16 * 2]
	ldp		x25, x26, [x0, #16 * 3]
	ldp		x27, x28, [x0, #16 * 4]
	ldp		fp, lr, [x0, #16 * 5]
	ldr		x8, [x0, #16 * 6]

	mov		sp, x8
	ret

/**
 * __fork64_return
 *
 * Invoke the last stage of the scheduler in the context switch, and branch to
 * the address stored in x19, which is expected to be the value of ELR_EL1
 * before the exception for the scheduler was taken. Resumes execution of the
 * previous thread.
 */
	.align	2
	.globl	__fork64_return
__fork64_return:
	mov		fp, xzr
	bl		sched_tail
	blr		x19
