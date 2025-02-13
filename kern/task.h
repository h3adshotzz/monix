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
 * 	Name:	task.h
 * 	Desc:	Task creation structure and definitions.
*/

#ifndef __KERN_TASK_H__
#define __KERN_TASK_H__

#include <kern/vm/vm_types.h>
#include <kern/vm/vm_map.h>
#include <kern/trace/printk.h>
#include <arch/arch.h>

#include <libkern/types.h>
#include <libkern/list.h>

/* Tasks status */
#define TASK_STATE_UNINITIALISED  (-1)
#define TASK_STATE_INACTIVE		(0)
#define TASK_STATE_ACTIVE		(1)

/* Maximum number of tasks */
#define TASK_COUNT_MAX			(12)

/* Maximum task name length */
#define TASK_NAME_MAX_LEN		(32)

/* Special task types */
typedef int						pid_t;
typedef int						task_state_t;
typedef vm_address_t			task_entry_t;

/**
 * Task structure
 * 
 * Represents a task. A task can contain multiple threads that all share the
 * same resources, as described by the task structure. Tasks are stored linked
 * together in a list_t, and a global task list is kept within task.c.
 * 
 * A number of things about a task is recorded, for example the total time
 * running, current execution time, priority, reference counting, etc.
 * 
 * A task has a single vm_map_t, which in-turn points to a pmap_t. All tasks run
 * in low-memory, whereas the kernel_task will run in high-memory.
*/
typedef struct task {

	pid_t				pid;		/* task id */
	task_state_t		state;		/* current state */

	/* task name, has a maximum length */
	char				name[TASK_NAME_MAX_LEN];

	/**
	 * Tasks are linked together in a doubly-linked list. The list head is kept
	 * within task.c. The task_t keeps a pointer to the head of a list of
	 * threads
	*/
	list_node_t			tasks;
	list_t				threads;

	/**
	 * Task's virtual memory map. This also points to the pmap_t structure,
	 * which contains the translation table information for this task.
	*/
	vm_map_t			*map;

	/**
	 * Timing statistic. The time the task has been executing since last being
	 * scheduled, and the total amount of time the task has been executing for.
	*/
	uint64_t			current_time;
	uint64_t			total_time;

	integer_t			priority;
	integer_t			preempt;

	/* Number of references */
	integer_t			ref_count;

} task_t;

/* Kernel task */
extern task_t			*kernel_task;

extern list_t			tasks;

extern void task_init();

extern task_t *get_current_task();
extern void *set_current_task(task_t *task);
extern task_t *task_create(vm_map_t *map, const char *name);

#endif /* __kern_task_h__ */
