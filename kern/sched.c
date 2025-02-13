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

#define pr_fmt(fmt)	"sched: " fmt

#include <kern/machine.h>
#include <kern/sched.h>
#include <kern/task.h>

void sched_init(void)
{
	// nothing to do here, yet.
}

/**
 * __select_thread
 * 
 * Logic for selecting the next thread to switch to. Currently, the next thread
 * is simply the next one in the global `threads` list, however eventually we'd
 * like to select them based on some kind of priority.
 */
static thread_t *__select_thread(thread_t *active_thread)
{
	if (list_is_last(&active_thread->threads, &threads)) {
		return (thread_t *) list_first_entry(&threads, struct thread, threads);
	}
	return (thread_t *) container_of(active_thread->threads.next, thread_t, threads);
}

/**
 * __schedule
 * 
 * Thread scheduler. Called when the timer interrupt is fired, and determines
 * the next task to switch to, and then managing that context switch.
*/
void __schedule(arm64_exception_frame_t *frame)
{
	thread_t *thread, *next_thread;
	cpu_t *cpu;

	machine_irq_disable();
	cpu = cpu_get_current();

	/**
	 * the next thread is selected via __select_thread, and then we verify if
	 * that thread is active. if it is, we can switch to it, otherwise we continue
	 * to select a thread until an active one is found.
	 */
	thread = cpu->cpu_active_thread;

	next_thread = __select_thread(thread);
	while (next_thread->state != THREAD_STATE_ACTIVE) {
		pr_debug("failed to select an active thread: %s.%d: %d\n",
			next_thread->task->name, next_thread->thread_id,
			next_thread->state);
		next_thread = __select_thread(thread);
	}

	pr_debug("switching to thread: %s.%d\n", next_thread->task->name,
		next_thread->thread_id);

	set_current_task(next_thread->task);
	thread_save_context(thread, frame);
	thread_load_context(next_thread);
}

/**
 * sched_tail
 * 
 * Scheduler tail. Called when returning from a context switch. Sets the new
 * active thread, enables preemption and unmasks interrupts.
*/
void sched_tail(thread_t *thread)
{
	/* idk why this needs to be done, but it does. for some fucking reason */
	vm_address_t stack = thread->stack;

	cpu_set_active_thread(machine_get_cpu_num(), thread);
	cpu_set_active_stack(machine_get_cpu_num(), stack);

	//thread_preempt_enable(thread);
	machine_irq_enable();
}
