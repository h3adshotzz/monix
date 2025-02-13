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
 *	Name:	cpu.c
 *	Desc:	Kernel CPU management interface. Handles interactions with physical
 *			CPUs, enable/disable, wakeup and other operations.
 */

#define pr_fmt(fmt)	"cpu: " fmt

#include <kern/defaults.h>
#include <kern/machine.h>
#include <kern/vm/pmap.h>
#include <libkern/panic.h>
#include <tinylibc/string.h>

/**
 * List of active CPUs. This array is allocated to the maximum number of allowed
 * CPUs (CPU_NUMBER_MAX), and is updated as each CPU becomes active.
 */
static cpu_t	CpuDataEntries[CPU_NUMBER_MAX];
static cpu_t	BootCpuData;

integer_t		cpu_count = 0;

/**
 * A special assertion to ensure a given cpu_id is within the bounds of the
 * current running system configuration.
*/
#define CPU_ASSERT_VALID_ID(__id)											\
	do {																	\
		if (__id > cpu_count) {												\
			panic("cpu: assertion failed: cpu_id '%d' is larger"			\
				"than count '%d'\n", __id, cpu_count);						\
		}																	\
	} while (0)

#define CPU_ASSERT_VALID(__cpu)												\
	do {																	\
		if (__cpu == NULL) {												\
			panic("cpu: assertion failed: invalid cpu data structure\n");	\
		}																	\
		if (__cpu->cpu_num >= CPU_NUMBER_MAX) {								\
			panic("cpu: assertion failed: invalid cpu_id '%d'\n",			\
				__cpu->cpu_num);											\
		}																	\
	} while (0)

/**
 * cpu_register
 * 
 * Register a cpu_t within the cpu data entries array.
*/
kern_return_t cpu_register(cpu_t *cpu_data_ptr)
{
	CPU_ASSERT_VALID(cpu_data_ptr);
	CpuDataEntries[cpu_data_ptr->cpu_num] = *cpu_data_ptr;

	/* no need to check anything here, as we passed the assertion */
	return KERN_RETURN_SUCCESS;
}

/**
 * cpu_create
 * 
 * Create and initialise a new cpu data structure from a given pointer. Should
 * be called during the early startup code for each cpu core.
*/
kern_return_t cpu_create(cpu_t *cpu_data_ptr, vm_address_t excepstack,
		vm_address_t intstack)
{
	memset(cpu_data_ptr, '\0', sizeof(cpu_t));

	cpu_data_ptr->cpu_flags = 0x0;
	cpu_data_ptr->cpu_reset_handler = (vm_address_t)
		mmu_translate_kvtop((vm_address_t) &_LowResetVector);

	cpu_data_ptr->excepstack_top = (vm_address_t) excepstack;
	cpu_data_ptr->intstack_top = (vm_address_t) intstack;

	cpu_data_ptr->cpu_num = machine_get_cpu_num();
	// todo: do cpu type, flag discovery

	// todo: setup interrupt fields, once that system has been reworked so the
	// irq handler calls the interrupt handler in the cpu data structure.

	return cpu_register(cpu_data_ptr);
}

/**
 * cpu_init
 * 
 * Initialise the cpu interface, collect, check and verify the boot cpu's arch
 * features and information.
*/
void cpu_init(void)
{
}

/**
 * cpu_halt
 * 
 * Halt the currently executing cpu.
*/
void cpu_halt(void)
{
	// todo: properly put the core to sleep
	__asm__ volatile ("b .");
}

/*----------------------------------------------------------------------------*/
// getter/setters

kern_return_t cpu_set_flag(cpu_number_t cpuid, uint32_t flag)
{
	CPU_ASSERT_VALID_ID(cpuid);

	CpuDataEntries[cpuid].cpu_flags |= flag;
	return KERN_RETURN_SUCCESS;
}

kern_return_t cpu_read_flag(cpu_number_t cpuid, uint32_t flag)
{
	CPU_ASSERT_VALID_ID(cpuid);

	return ((CpuDataEntries[cpuid].cpu_flags & flag) == flag) ?
		KERN_RETURN_FAIL : KERN_RETURN_SUCCESS;
}

kern_return_t cpu_set_active_stack(cpu_number_t cpuid, vm_address_t stack)
{
	CPU_ASSERT_VALID_ID(cpuid);

	CpuDataEntries[cpuid].cpu_active_stack = stack;
	return KERN_RETURN_SUCCESS;
}

kern_return_t cpu_set_active_thread(cpu_number_t cpuid, thread_t *thread)
{
	CPU_ASSERT_VALID_ID(cpuid);

	if (thread == THREAD_NULL)
		panic("cpu: failed to set cpu '%d' active thread: invalid thread\n",
			cpuid);

	CpuDataEntries[cpuid].cpu_active_thread = thread;
	return KERN_RETURN_SUCCESS;
}

kern_return_t cpu_set_processor(cpu_number_t cpuid, processor_t *processor)
{
	CPU_ASSERT_VALID_ID(cpuid);
	CpuDataEntries[cpuid].processor = processor;
	return KERN_RETURN_SUCCESS;
}

kern_return_t cpu_set_boot_cpu(cpu_t *cpu)
{
	CPU_ASSERT_VALID(cpu);
	BootCpuData = *(cpu_t *) cpu;
	CPU_ASSERT_VALID(((cpu_t *)&BootCpuData));

	return KERN_RETURN_SUCCESS;
}

cpu_t *cpu_get_cpu(cpu_number_t cpuid)
{
	return (cpu_t *) &CpuDataEntries[cpuid];
}

cpu_t *cpu_get_current(void)
{
	return cpu_get_cpu(machine_get_cpu_num());
}

processor_t *cpu_get_processor(cpu_number_t cpuid)
{
	cpu_t *cpu = cpu_get_cpu(cpuid);
	CPU_ASSERT_VALID(cpu);

	return cpu->processor;
}

cpu_t *processor_get_cpu(processor_t *processor)
{
	cpu_t *cpu;

	CPU_ASSERT_VALID_ID(processor->cpu_id);

	cpu = (cpu_t *) cpu_get_cpu(processor->cpu_id);
	CPU_ASSERT_VALID(cpu);

	return cpu;
}
