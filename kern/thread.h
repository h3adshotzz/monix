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
 * 	Name:	thread.h
 * 	Desc:	Thread creation structure and definitions.
*/

#ifndef __KERN_THREAD_H__
#define __KERN_THREAD_H__

#include <arch/arch.h>
#include <kern/task.h>

#include <libkern/list.h>

typedef struct arm64_cpu_context	cpu_context_t;

/* Special thread types */
typedef vm_address_t				thread_entry_t;

/* should move this to defaults, along with the task max */
#define THREAD_COUNT_MAX			24

/* maximum length of a threads name */
#define THREAD_NAME_MAX_LEN			64

/* default size of a stack */
#define THREAD_STACK_DEFAULT_SIZE	VM_PAGE_SIZE

#define THREAD_NULL					NULL

/**
 * To easily identify it, the kernel thread MUST be the first one created, and
 * must have the thread_id '0'
 */
#define THREAD_ID_KERN_THREAD		(0)

#define THREAD_PRIORITY_MAX			(4)
#define THREAD_PRIORITY_LOW			(0)

/**
 * Thread structure
 * 
 * Represents a single thread. A thread shares the resources of a parent task,
 * which holds a list of all child threads. The thread contains a pointer to the
 * parent task.
*/
typedef struct thread {

	/**
	 * When switching threads the schedular will save teh callee registers to
	 * the threads context, and load it when switching back. This is instead of
	 * saving them on the stack.
	 * 
	 * The context is the first field in the thread structure, purely for
	 * simplicity, as the context switcher is written in assembly.
	*/
	cpu_context_t	context;
	
	vm_address_t	stack_base;
	vm_address_t	stack;

	/* Thread identifier */
	pid_t			thread_id;

	list_node_t		siblings;		// other threads in the same task
	list_node_t		threads;		// global list of threads

	/* Parent task */
	task_t			*task;

	/* Thread entry */
	thread_entry_t	entry;
	void			*args;

	/* Preemption */
	integer_t		preempt;

	/* Flags */
	uint32_t
	
#define THREAD_STATE_INACTIVE	(0x0)
#define THREAD_STATE_ACTIVE		(0x1)
	/* boolean_t */	state		:1,		/* thread state */

	/* future */	reserved	:30;	/* reserved */

	/* Reference counter */
	integer_t		ref_count;

	/**
	 * Statistics. These are much the same as kept in task_t, but for this
	 * individual thread, rather than the whole task.
	*/
	uint64_t		current_time;
	uint64_t		total_time;

	/* thread name */
	char		name[THREAD_NAME_MAX_LEN];

} thread_t;

extern list_t threads;

extern kern_return_t thread_init();
extern kern_return_t thread_destroy(thread_t *thread);

extern thread_t *thread_create(task_t *parent_task, integer_t priority, thread_entry_t entry, const char *name);
extern thread_t *kernel_thread_create(thread_entry_t entry, integer_t priority, void *args);

// todo
extern kern_return_t thread_destroy(thread_t *thread);

extern kern_return_t thread_block();
extern kern_return_t thread_unblock();

extern void thread_set_name(thread_t *thread, const char *name);

extern void thread_load_context(thread_t *thread);
extern void thread_save_context(thread_t *thread, arm64_exception_frame_t *frame);
extern kern_return_t task_assign_thread(task_t *task, thread_t *thread);

extern thread_t *thread_get_current();

#endif /* __kern_thread_h__ */
