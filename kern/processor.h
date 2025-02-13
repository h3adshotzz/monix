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
 *	Name:	processor.h
 *	Desc:	Architecture-agnostic processor interface.
*/

#ifndef __KERN_PROCESSOR_H__
#define __KERN_PROCESSOR_H__

#include <kern/thread.h>
#include <kern/trace/printk.h>
#include <tinylibc/stdint.h>
#include <libkern/list.h>

/**
 * 							Processor Interface
 * 
 * 	There are two interfaces within the kernel that describe a single processor
 * 	or cpu core; the cpu.h interface, and processor.h interface. The cpu.h is an
 * 	architecture-specific representation of a CPU, and covers properties such as
 * 	stacks, interrupts, timings, system registers, etc. Whereas processor.h is
 * 	a slightly higher-level, architecture-agnostic representation of the CPU.
 * 
 * 	The processor_t and cpu_data_t aren't strongly linked, by that I mean there
 * 	is no pointer from the processor to the cpu data. Instead, the processor
 * 	simply keeps track of the cpuid which is represents, and the cpu interface
 * 	can translate a processor_t to cpu_data_t based on this cpuid.
 * 
 * 	Although this takes inspiration from Mach/XNU, we do not use a processor_set
 * 	structure. Instead, these values (active, idle queues, counts, etc) are just
 * 	static globals within processor.c.
 * 
 * 	The processor interface is initialised via processor_init(). This will setup
 * 	the necessary queues, identify and create processor_t structs for all CPUs
 * 	on the system. It will then set the boot cpu as the primary processor.
 * 
 * 	NOTES:		- could some initial smp config be done here? use this for proc
 * 				  detection?
 * 				- proc detection would be done via device tree discovery
 * 				- processor.h could act as a wrapper for cpu bringup around
 * 				  cpu.h
*/

#define PROCESSOR_STATE_INACTIVE		(0x0)
#define PROCESSOR_STATE_ACTIVE			(0x1)
#define PROCESSOR_STATE_IDLE			(0x2)

typedef struct processor {

	/**
	 * A processor keeps track of three threads: the active one, the one we'll
	 * next switch to, and the one to switch to when idling.
	*/
	thread_t		*active_thread;
	thread_t		*next_thread;
	thread_t		*idle_thread;

	/**
	 * The current threads priority, as well as the cpu id. The cpuid must
	 * correspond with a cpu_t structure.
	*/
	integer_t		priority;
	integer_t		cpu_id;

	/* processor flags */
	integer_t		state		:1,
					__unused;

	/* processor list node */
	list_node_t		proc_list;

} processor_t;

extern void processor_init(void);

extern processor_t *current_processor(void);

extern processor_t *processor_create(integer_t cpu_id);
extern void processor_destroy(processor_t *processor);

#endif /* __kern_processor_h__ */
