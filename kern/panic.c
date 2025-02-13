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

#include <libkern/version.h>
#include <libkern/panic.h>

#include <tinylibc/stddef.h>

#include <kern/trace/printk.h>
#include <kern/machine.h>
#include <kern/task.h>

extern void kernel_init(struct boot_args *boot_args, uint64_t x1, uint64_t x2);

static void __panic(int flags, void *frame, const char *fmt, ...);
static void __print_backtrace(cpu_t *panic_cpu);

static int panic_active = 0;

#define PANIC_FLAG_NONE		0
#define PANIC_FLAG_CPUTRACE	1

#define PANIC_FRAME_NONE	NULL

/* simple panic, no thread state */
void panic(const char *fmt, ...)
{
	__panic(PANIC_FLAG_NONE, PANIC_FRAME_NONE, fmt);
}

/* panic with thread state */
void panic_with_thread_state(arm64_exception_frame_t *frame, const char *fmt, ...)
{
	__panic(PANIC_FLAG_CPUTRACE, (void *)frame, fmt);
}

/**
 * __panic
 * 
 * Monix kernel panic handler. This is not exposed outside of this file, instead
 * two symbols are exposed: panic and panic_with_thread_state. 
 */
static void __panic(int flags, void *frame, const char *fmt, ...)
{
	arm64_exception_frame_t *_frame;
	thread_t *panic_thread;
	cpu_t *panic_cpu;
	int panic_pid = -1, elx;
	va_list args;

	/**
	 * Sometimes a panic can occur within the panic handler, to avoid this check
	 * the panic_active value and if that is set, jump straight to cpu_halt.
	 */
	if (panic_active)
		goto cpu_halt;

	panic_active = 1;

	/**
	 * Disable interrupts, the kernel cannot recover from this state and the
	 * panic handler needs to be able to complete and then halt the cpu.
	 */
	machine_irq_disable();

	/**
	 * We assume that it's the currently active CPU that has panicked, so obtain
	 * the cpuid and fetch the pid of the currently active thread.
	 */
	panic_cpu = cpu_get_current();
	panic_thread = panic_cpu->cpu_active_thread;
	if (get_current_task() != 0)
		panic_pid = get_current_task()->pid;

	/* Begin panic handler log */
	kprintf("\n");
	kprintf("--- Kernel Panic - ");
	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
	pr_cont("\n");

	kprintf("CPU: %d  PID: %d  %s:%s/%s_%s\n", panic_cpu->cpu_num, panic_pid,
		DEFAULTS_KERNEL_BUILD_MACHINE, KERNEL_SOURCE_VERSION,
		KERNEL_BUILD_STYLE, KERNEL_BUILD_TARGET);
	kprintf("Machine: tiny-ex1\n");	/* todo: machine should store this */
	kprintf("Kernel:  Monix Kernel Version %s; %s\n", KERNEL_BUILD_VERSION, __TIMESTAMP__);

	/* Thread backtrace */
	__print_backtrace(panic_cpu);

	/* CPU state */
	if (flags & PANIC_FLAG_CPUTRACE) {
		_frame = (arm64_exception_frame_t *) frame;

		kprintf("CPU State:\n");
		kprintf("  x0: 0x%016llx   x1: 0x%016llx   x2: 0x%016llx   x3: 0x%016llx\n",
			_frame->regs[0], _frame->regs[1], _frame->regs[2], _frame->regs[3]);
		kprintf("  x4: 0x%016llx   x5: 0x%016llx   x6: 0x%016llx   x7: 0x%016llx\n",
			_frame->regs[4], _frame->regs[5], _frame->regs[6], _frame->regs[7]);
		kprintf("  x8: 0x%016llx   x9: 0x%016llx  x10: 0x%016llx  x11: 0x%016llx\n",
			_frame->regs[8], _frame->regs[9], _frame->regs[10], _frame->regs[11]);
		kprintf(" x12: 0x%016llx  x13: 0x%016llx  x14: 0x%016llx  x15: 0x%016llx\n",
			_frame->regs[12], _frame->regs[13], _frame->regs[14], _frame->regs[15]);
		kprintf(" x16: 0x%016llx  x17: 0x%016llx  x18: 0x%016llx  x19: 0x%016llx\n",
			_frame->regs[16], _frame->regs[17], _frame->regs[18], _frame->regs[19]);
		kprintf(" x20: 0x%016llx  x21: 0x%016llx  x22: 0x%016llx  x23: 0x%016llx\n",
			_frame->regs[20], _frame->regs[21], _frame->regs[22], _frame->regs[23]);
		kprintf(" x24: 0x%016llx  x25: 0x%016llx  x26: 0x%016llx  x27: 0x%016llx\n",
			_frame->regs[24], _frame->regs[25], _frame->regs[26], _frame->regs[27]);
		kprintf(" x28: 0x%016llx   fp: 0x%016llx   lr: 0x%016llx   sp: 0x%016llx\n",
			_frame->regs[28], _frame->fp, _frame->lr, _frame->sp);
		kprintf("\n");

		elx = (sysreg_read(currentel) >> 2);
		kprintf("Exception taken at EL%d\n", elx);
		kprintf("  FAR_EL%d: 0x%016llx (0x%08lx)\n", elx, ptokva(_frame->far), _frame->far);
		kprintf("  ESR_EL%d: 0x%016llx\n", elx, _frame->esr);
		kprintf("\n");
	}

	kprintf("Kernel base: 0x%lx\n", kernel_init);
	kprintf("\n");

	kprintf("---[end Kernel Panic - ");
	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
	pr_cont(" ]\n");

cpu_halt:
	cpu_halt();
	/*NORETURN*/
}

/* dump the backtrace */
static void __print_backtrace(cpu_t *panic_cpu)
{
	struct frame_record *fr;
	vm_address_t frame_address;

	/**
	 * It's possible the kernel could crash before the main thread has started,
	 * so check there is an active thread before trying to output it's info.
	 */
	if (cpu_read_flag(panic_cpu->cpu_num, CPU_FLAG_THREADING_ENABLED)) {
		kprintf("Process name: %s  Thread ID: %d\n",
			panic_cpu->cpu_active_thread->task->name,
			panic_cpu->cpu_active_thread->thread_id);
	} else {
		kprintf("Kernel faulted before main thread enabled\n");
	}
	kprintf("\n");

	/* Print the backtrace */
	kprintf ("Backtrace (CPU%d):\n", panic_cpu->cpu_num);

	__asm__ volatile ("mov %0, x29" : "=r" (frame_address));
	fr = (struct frame_record *) frame_address;
	for (int i = 0; i < 20; i++) {
		/* basic check as to whether the record is valid */
		if (fr == NULL || fr->parent == NULL)
			break;

		kprintf("\t%d: 0x%lx\n", i, fr->return_addr);
		fr = fr->parent;
	}
	kprintf("\n");
}
