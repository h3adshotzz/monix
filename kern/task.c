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

#define pr_fmt(fmt)	"task: " fmt

#include <kern/task.h>
#include <kern/sched.h>
#include <kern/thread.h>
#include <kern/trace/printk.h>
#include <kern/defaults.h>
#include <kern/vm/vm_page.h>
#include <kern/mm/zalloc.h>

#include <tinylibc/string.h>

#include <libkern/list.h>
#include <libkern/panic.h>

/* Toggle stack guard check */
#define TASK_DO_STACK_GUARD_CHECK		DEFAULTS_DISABLE

/**
 * A reference to the kernel task is stored here, along with the task list head
 * and PID count.
*/
task_t		*kernel_task; //= &kernel_task_ref;

integer_t	task_pid = 0;

/**
 * For now, we'll manually track where the allocations are, but we should switch
 * to some kind of zone allocator or something, so tasks can be allocated in
 * the same place.
*/
zone_t			*task_zone;
vm_offset_t		task_page_cursor = 0;

/**
 * NOTE:	For the moment, we'll record the current task here, so the panic
 * 			handler doesn't fuck up. But this needs to change in two ways:
 * 				- first, the current thing executing should be in the cpu data
 * 				- second, the cpu data should have the current thread, which in
 * 				  turn points to the current task.
 * 
 * 			So, when calling task_get_current(), this will actually fetch the 
 * 			current cpu_t, extract the thread, and access thread->task. Instead
 * 			of trying to track the current task here too. But the kernel task
 * 			will stay here, as that never changes.
*/
task_t		*current_task;

/******************************************************************************/
//
// THIS AREA IS TEMPORARY, AND IS REQUIRED TO STOP THINGS CRASHING WITHOUT THE
// THREADS IMPLEMENTATION.
//


task_t *get_current_task()
{
	return current_task;
}

void *set_current_task(task_t *task)
{
	current_task = task;
}

/**
 * _dump_tasks
*/
void _dump_tasks()
{
	task_t *entry;

	pr_info("dumping global task list information:\n");

	list_for_each_entry(entry, &tasks, tasks) {
		vm_address_t stack_guard_addr;
		const char *stack_guard = "__STACK_GUARD__";

		kprintf("task[%d]: pid '%d', name '%s':\n", entry->pid, entry->pid, entry->name);

		/**
		 * Loop through the list of threads assigned to this task.
		*/
		thread_t *thread;
		list_for_each_entry(thread, &entry->threads, siblings) {
			kprintf("    thread[%d]: stack: 0x%lx, entry: 0x%lx\n",
				thread->thread_id, thread->stack, thread->entry);
		}
		if (list_empty(&entry->threads)) {
			kprintf("    no threads on task\n");
		}

		/**
		 * This isn't intended to stay here, it's just to test that the stack
		 * for each task is valid. If it's not, the kernel will crash.
		*/
#if TASK_DO_STACK_GUARD_CHECK
		stack_guard_addr = entry->context.sp;
		memcpy(stack_guard_addr, stack_guard, strlen(stack_guard));

		kprintf_hexdump(stack_guard_addr, stack_guard_addr, 64);
#endif
	}
}

/******************************************************************************/

/**
 * task_init
 * 
 * Setup the global task list, then create and register the first task for the
 * kernel.
*/
void task_init()
{
	/**
	 * create a zone for the task structures, with the maximum allowed number of
	 * tasks.
	*/
	task_zone = zone_create(sizeof(task_t),
					TASK_COUNT_MAX * sizeof(task_t),
					"task_zone");

	/* create the kernel task as the first task in the list */
	kernel_task = task_create(vm_get_kernel_map(), "kernel_task");

	pr_info("task_init complete\n");
}


/**
 * task_create
 * 
 * Creates a new task_t with a given entry point and vm_map. The task is placed
 * within the task page, and a new stack is allocated on the given map. The task
 * is added to the global tasks list, and a pid is assigned.
*/
task_t *task_create(vm_map_t *map, const char *name)
{
	size_t	name_len;
	task_t	*new;

	/* allocate an element within the task_zone for this task struct */
	new = (task_t *) zalloc(task_zone);
	new->ref_count = 2;

	new->state = TASK_STATE_INACTIVE;
	new->pid = task_pid;
	task_pid += 1;

	/* copy the task name */
	if ((name_len = strlen(name)) > TASK_NAME_MAX_LEN)
		name_len = TASK_NAME_MAX_LEN;
	memset(new->name, '\0', name_len + 1);
	memcpy(new->name, name, name_len);

	/* initialise the list for the tasks threads */
	INIT_LIST_HEAD(&new->threads);
	list_add_tail(&new->tasks, &tasks);

	return new;
}

/**
 * 
*/
kern_return_t task_assign_thread(task_t *task, thread_t *thread)
{
	// todo: verify the thread, and the task before adding it
	pr_info("attaching thread '%d' to task '%s'\n", thread->thread_id, task->name);

	list_add_tail(&thread->siblings, &task->threads);
	thread->task = task;
}
