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
 *	Name:	cpu.h
 *	Desc:	Kernel CPU management interface. Handles interactions with physical
 *			CPUs, enable/disable, wakeup, and other operations.
 *
 */

#ifndef __KERN_CPU_H_
#define __KERN_CPU_H__

#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/defaults.h>
#include <kern/vm/vm_types.h>
#include <kern/trace/printk.h>

#include <libkern/types.h>
#include <tinylibc/stdint.h>

/* CPU typedefs */
typedef int			cpu_number_t;
typedef int			cpu_state_t;
typedef int			cpu_type_t;

/* Invalid CPU number */
#define CPU_NUMBER_INVALID		(-1)
#define CPU_NUMBER_MAX			DEFAULTS_MACHINE_MAX_CPUS

/* CPU Flags */
#define CPU_FLAG_THREADING_ENABLED	(1 << 0)	/* Has threading been enabled yet? */

/** TOOD: Move to interrupt handler header */
typedef void (*irq_handler_t) (unsigned int source);

/**
 * CPU Data
 *
 * This can be referred to as either the "CPU structure" or "CPU Data structure",
 * it defines the current state of a CPU in the system.
 */
typedef struct cpu_data
{
	cpu_number_t		cpu_num;
	cpu_type_t			cpu_type;
	uint32_t			cpu_flags;

	processor_t			*processor;

	/* Interrupt handling */
	vm_address_t		excepstack_top;
	vm_address_t		intstack_top;

	unsigned int		interrupt_source;
	unsigned int		interrupt_state;
	irq_handler_t		interrupt_handler;

	/* Reset */
	vm_address_t		cpu_reset_handler;

	/* Thread */
	thread_t			*cpu_active_thread;
	vm_address_t		cpu_active_stack;

	uint64_t			cpu_tpidr_el0;

} cpu_t;

/**
 * External definitions for start.S functions: Low reset and exception vectors
 */
extern vm_address_t			_LowResetVector;
extern vm_address_t			_LowExceptionVectorBase;

/* CPU API */
extern void cpu_init(void);
extern void cpu_halt(void);

extern kern_return_t cpu_create(cpu_t *cpu_data_ptr, vm_address_t excepstack,
								vm_address_t intstack);
extern kern_return_t cpu_destroy(cpu_t *cpu_data_ptr);

extern kern_return_t cpu_register(cpu_t *cpu_data_ptr);
extern kern_return_t cpu_set_boot_cpu(cpu_t *cpu);

extern kern_return_t cpu_set_flag(cpu_number_t cpuid, uint32_t flag);
extern kern_return_t cpu_read_flag(cpu_number_t cpuid, uint32_t flag);

extern kern_return_t cpu_set_active_thread(cpu_number_t cpuid, thread_t *thread);
extern kern_return_t cpu_set_active_stack(cpu_number_t cpuid, vm_address_t stack);
extern kern_return_t cpu_set_processor(cpu_number_t cpuid, processor_t *processor);

extern cpu_t *cpu_get_current(void);
extern cpu_t *cpu_get_cpu(cpu_number_t cpuid);

extern processor_t *cpu_get_processor(cpu_number_t cpuid);
extern cpu_t *processor_get_cpu(processor_t *processor);

#endif /* __kern_cpu_h__ */
