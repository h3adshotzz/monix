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

#define pr_fmt(fmt)	"thread: " fmt

#include <kern/sched.h>
#include <kern/thread.h>
#include <kern/machine.h>
#include <kern/vm/vm.h>
#include <kern/vm/vm_page.h>
#include <kern/mm/zalloc.h>
#include <kern/mm/stack.h>

#include <libkern/panic.h>

#include <tinylibc/string.h>

/* thread zone */
zone_t		*thread_zone;

integer_t	thread_id_max = 0;

static kern_return_t thread_init_context(thread_t *thread, thread_entry_t *entry);

void _dump_threads()
{
	thread_t *entry;

	pr_debug("dumping global thread list information:\n");

	list_for_each_entry(entry, &threads, threads) {
		pr_debug("thread[%d]: id '%d', task name '%s':\n",
			entry->thread_id, entry->thread_id, entry->task->name);
	}
}

/**
 * thread_init
 * 
 * Setup the thread's zone, and initialise the stack allocator
*/
kern_return_t thread_init()
{
	thread_zone = zone_create(sizeof(thread_t),
			THREAD_COUNT_MAX * sizeof(thread_t),
			"thread_zone");

	stack_init();
}

/**
 * thread_create
 * 
 * Create a new thread_t with a given entry point and schedular priority, and
 * assign it to a specified task. 
*/
thread_t *thread_create(task_t *parent_task, integer_t priority,
		thread_entry_t entry, const char *name)
{
	vm_address_t stack;
	thread_t *thread;

	/**
	 * Thread structures are allocated within the thread_zone in kernel memory,
	 * so the actual userspace process cannot read it's own thread/task
	 * structure.
	*/
	thread = (thread_t *) zalloc(thread_zone);

	/* initial state is inactive */
	thread->stack = THREAD_STATE_INACTIVE;

	/**
	 * initial values for the thread: references, preemption, and thread_id.
	*/
	thread->ref_count = 2;
	thread->preempt = 0;
	
	thread->thread_id = thread_id_max;
	thread_id_max+=1;

	/* allocate a stack */
	stack_alloc(thread);

	/* initial cpu context */
	thread_init_context(thread, (thread_entry_t *) entry);

	/* assign to parent task */
	task_assign_thread(parent_task, thread);

	/* assign thread to global list */
	list_add_tail(&thread->threads, &threads);

	/* set the threads name */
	thread_set_name(thread, name);

	/* thread can be considered active from this point */
	thread->state = THREAD_STATE_ACTIVE;

	return thread;
}

/**
 * thread_destroy
 * 
 * Destroy an existing thread, deallocate any memory associated with it and
 * remove it from the scheduler list.
 */
kern_return_t thread_destroy(thread_t *thread)
{
	char tname[THREAD_NAME_MAX_LEN];

	/* cannot be interrupted while doing this */
	thread_block();

	strlcpy(tname, thread->name, THREAD_NAME_MAX_LEN);

	/**
	 * cannot destroy the active thread, if we're trying to do this, something
	 * big has fucked up.
	*/
	if ((vm_address_t *) thread == (vm_address_t *) cpu_get_current()->cpu_active_thread) {
		panic("cannot destroy active thread\n");
	}
	pr_debug("destroying: %s (%s.%d)\n", tname, thread->task->name,
		thread->thread_id);

	/* deactivate the thread so the scheduler will ignore it */
	thread->state = THREAD_STATE_INACTIVE;

	/* remove the thread from the siblings and global lists */
	list_del(&thread->siblings);
	list_del(&thread->threads);

	/* free the thread's stack in the stack_zone */
	stack_free(thread);

	pr_info("destroyed thread '%s' (%s.%d)\n", tname,
		thread->task->name, thread->thread_id);

	/* free the thread in the thread_zone */
	zfree(thread_zone, (vm_address_t) thread);

	/* unblock the current thread */
	thread_unblock();

}

/**
 * thread_block
 * 
 * Block the scheduler from switching to another thread until thread_unblock is
 * called. Holds the system on the current thread.
 */
kern_return_t thread_block()
{
	machine_irq_disable ();
}

/**
 * thread_unblock
 * 
 * Unblock the scheduler from switching to another thread.
 */
kern_return_t thread_unblock()
{
	machine_irq_enable();
}

/**
 * kernel_thread_create
 *
 * Create the kernel thread and configure it with the given entry point and
 * initial arguments. The thread is assigned to the kernel task, and a pointer
 * to the thread structure is returned.
 */
thread_t *kernel_thread_create(thread_entry_t entry, integer_t priority,
		void *args)
{
	thread_t *thread;

	/* create the new thread */
	thread = thread_create(kernel_task, priority, entry, "kthread_main");
	if (thread == THREAD_NULL)
		panic("kernel_thread_create: failed to create kernel thread\n");

	/* the kernel thread MUST be THREAD_ID_KERN_THREAD */
	if (thread->thread_id != THREAD_ID_KERN_THREAD)
		panic("kernel_thread_create: kernel thread not created first\n");

	/* set the thread arguments */
	thread->args = args;
	thread->state = THREAD_STATE_ACTIVE;

	pr_info("created kernel thread '0x%lx' at entry: 0x%lx\n", thread, entry);
	pr_debug("thread->task: 0x%lx\n", thread->task);
	return thread;
}

/**
 * thread_set_name
 * 
 * Reset the name of a given thread.
 */
void thread_set_name(thread_t *thread, const char *name)
{
	size_t len;

	len = (len < THREAD_NAME_MAX_LEN) ? len : THREAD_NAME_MAX_LEN;
	strlcpy(thread->name, name, len);
}

// todo: sets up the threads context, prep the entry point and args
static kern_return_t thread_init_context(thread_t *thread,
		thread_entry_t *entry)
{
	thread->entry = (thread_entry_t) entry;
	// todo: work out how thread entry will work, what needs to be in which
	// registers
}

/**
 * thread_load_context
 * 
 * Load the context of a given thread onto the current cpu. Calling this
 * function will have the affect of switching to the given thread.
*/
void thread_load_context(thread_t *thread)
{
	pr_debug("load_context: address: 0x%lx, stack: 0x%lx\n",
		thread->context.lr, thread->stack);

	/**
	 * Restore specific registers to prepare the thread context to be copied to
	 * the cpu registers. x19 holds the entry point upon re-entering the thread,
	 * therefore should contain the ELR_EL1 value of the thread before it was
	 * interrupted. x20 holds any arguments that need to be passed to the thread,
	 * typically only used on first execution.
	 * 
	 * The stack is saved in thread->stack, and is restored. __fork64_return
	 * handles entering the thread when it's first executed, when re-entering an
	 * active thread, this is just ignored.
	*/
	thread->context.x19 = (uint64_t) thread->entry;
	thread->context.x20 = (uint64_t) thread->args;
	thread->context.sp = (uint64_t) thread->stack;
	thread->context.lr = (uint64_t) __fork64_return;

	/**
	 * __fork64_exec will complete the scheduler process, and jump to the address
	 * in x19.
	*/
	__fork64_exec(thread);

	/*NOTRETURN*/
}

/**
 * thread_save_context
 * 
 * Saves the current cpu context to the currently active thread, using the frame
 * generated by the exception handler.
*/
void thread_save_context(thread_t *thread, arm64_exception_frame_t *frame)
{
	/* save the callee registers from the exception frame */
	thread->context.x19 = frame->regs[19];
	thread->context.x20 = frame->regs[20];
	thread->context.x21 = frame->regs[21];
	thread->context.x22 = frame->regs[22];
	thread->context.x23 = frame->regs[23];
	thread->context.x24 = frame->regs[24];
	thread->context.x25 = frame->regs[25];
	thread->context.x26 = frame->regs[26];
	thread->context.x27 = frame->regs[27];
	thread->context.x28 = frame->regs[28];

	/* save the frame pointer and stack pointer from the exception frame */
	thread->context.fp = frame->fp;
	thread->context.sp = frame->sp;

	/* read the ELR_EL1 to determine the re-entry point for this thread */
	thread->entry = sysreg_read(elr_el1);
	thread->stack = thread->context.sp;
}
