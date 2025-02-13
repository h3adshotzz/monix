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
 * Name:	main.c
 * Desc:	Kernel startup code. Initial entry point from assembly, execution
 * 			continues here until virtual memory and tasking is setup, and then
 * 			we jump to the kernel_task.
 */

/* tinylibc */
#include <tinylibc/stdint.h>
#include <tinylibc/byteswap.h>
#include <tinylibc/string.h>

/* libkern */
#include <libkern/boot.h>
#include <libkern/assert.h>
#include <libkern/version.h>
#include <libkern/panic.h>
#include <libkern/list.h>

/* kernel */
#include <kern/machine.h>
#include <kern/machine/machine-irq.h>
#include <kern/machine/machine_timer.h>
#include <kern/trace/printk.h>
#include <kern/vm/vm.h>
#include <kern/vm/pmap.h>
#include <kern/vm/vm_page.h>
#include <kern/mm/zalloc.h>
#include <kern/processor.h>
#include <kern/task.h>

/* platform */
#include <platform/devicetree.h>
#include <platform/platform.h>

/* arch */
#include <arch/arch.h>


/**
 * Interrupt and Exception stack pointers
 */
extern vm_address_t intstack_top;
extern vm_address_t excepstack_top;

extern vm_address_t kernel_task_stack;

static task_t *test_task_ptr;

static uint32_t boot_cpu_id;

#define kthread_log(fmt, ...)	\
	_printk(LOGLEVEL_DEFAULT, 0, __strfmt("kthread: " fmt), ##__VA_ARGS__)

/* Symbols from info.c */
extern void print_boot_banner(const DTNode *dt_root, cpu_number_t cpu_num,
	const char *tboot_vers, void *entry);


void kernel_thread_main(const void *arg1);

/**
 * The kernel will enter here from start.S and will complete the necessary setup
 * until the kernel_task can be launched, at which point the .startup section
 * will be erased from memory and unampped.
*/
void kernel_init(struct boot_args *boot_args, uint64_t x1, uint64_t x2)
{
	const DTNode *dt_root;
	phys_addr_t membase;
	phys_size_t memsize;
	uint64_t x0, len;
	cpu_t boot_cpu;
	char *machine;

	/* initialise the cpu_data for the boot cpu */
	cpu_create(&boot_cpu, (vm_address_t) &intstack_top,
		(vm_address_t) &excepstack_top);

	/* verify the boot parameters */
	if (boot_args->version != BOOT_ARGS_VERSION_1_1)
		panic("boot_args version mismatch\n");

	/* convert the fdt base to a virtual address */
	if (boot_args->fdtbase < boot_args->virtbase)
		boot_args->fdtbase = (uint64_t) (boot_args->virtbase + 
			(boot_args->fdtbase - boot_args->physbase));

	/* initialise the device tree */
	DeviceTreeInit ((void *) boot_args->fdtbase, boot_args->fdtsize);
	dt_root = BootDeviceTreeGetRootNode ();

	/* update the address of the boot args struct */
	x0 = (uint64_t) boot_args->virtbase + (((uint64_t) boot_args) - ((uint64_t) boot_args->physbase)); //(boot_args->virtbase + (x0 - boot_args->physbase));
	boot_args = (struct boot_args *) x0;

	/* fetch platform memory layout and setup virtual memory */
	platform_get_memory (&membase, &memsize);
	arm_vm_init (boot_args, membase, memsize);

	/* initialise the console */
	console_setup();

	/* now we have logs, verify the device tree */
	DeviceTreeVerify();

	/* parse the machine cpu topology */
	machine_parse_cpu_topology();
	boot_cpu.cpu_num = machine_get_boot_cpu_num();
	assert (boot_cpu.cpu_num <= machine_get_max_cpu_num());

	boot_cpu_id = boot_cpu.cpu_num;

	cpu_register(&boot_cpu);
	cpu_set_boot_cpu(&boot_cpu);

	/* cpu initialisation */
	cpu_init();

	/* boot banner */
	kprintf("Booting Monix on Physical CPU: 0x%08llx [0x%llx]\n", boot_cpu.cpu_num, kernel_init);
	kprintf("Monix Kernel Version %s; %s; %s:%s/%s_%s\n", KERNEL_BUILD_VERSION, __TIMESTAMP__,
		DEFAULTS_KERNEL_BUILD_MACHINE, KERNEL_SOURCE_VERSION, KERNEL_BUILD_STYLE, KERNEL_BUILD_TARGET);
	kprintf("tBoot version: %s\n", boot_args->tboot_vers);

	DeviceTreeLookupPropertyValue (*dt_root, "compatible", &machine, &len);
	kprintf("machine: %s\n", machine);
	kprintf("machine: detected '%d' cpus across '%d' clusters\n", machine_get_num_cpus (), machine_get_num_clusters ());

	/* configure remaining virtual memory subsystems */
	vm_configure();

	/* configure the zone allocator */
	zone_init();

	/* enable interrupts */
	machine_init_interrupts();

	/* processor init */
	processor_init();

	/* task init, creates the kernel_task */
	task_init();

	/* thread init */
	thread_init();

	/* create the main kernel thread */
	thread_t *thread = kernel_thread_create((thread_entry_t)kernel_thread_main,
				THREAD_PRIORITY_MAX, THREAD_NULL);
	kprintf("kthread created\n");

	// testing
	_dump_tasks();
	vm_pagetable_walk_ttbr1();

	thread_load_context(thread);

	/*NOTREACHED*/
	__asm__ volatile ("b .");
}

/**
 * Threading tests
 * 
 * The following functions and definitions, with exception of kernel_thread_main,
 * are used to test the context switching logic. These will be removed once the
 * kernel is actually using threads for a specific purpose.
 */

void test_thread_1();
int counter = 0;
int thread_destroyed = 0;

/**
 * Kernel thread.
 */
void kernel_thread_main(const void *arg1)
{
	kthread_log("\n\nkernel_task: kernel_init complete\n");

	/* allow cpu_active_thread to be accessed */
	cpu_set_flag(boot_cpu_id, CPU_FLAG_THREADING_ENABLED);

	/* create dummy threads */
	thread_t *test_thread = 
		thread_create(kernel_task, THREAD_PRIORITY_LOW, (thread_entry_t)test_thread_1, "test_thread_1");

	_dump_threads();

	/* initialise timers to allow for scheduling */
	machine_init_timers();

	cpu_t *cpu = cpu_get_current();
	thread_t *thread = cpu->cpu_active_thread;
	kthread_log("cpu[%d]: %s.%d\n", cpu->cpu_num, thread->task->name, thread->thread_id);

	while (1) {
		int interval = 70000000;
		while (interval--);

		kthread_log("kthread_main: hello world\n");

		if (counter >= 5 && !thread_destroyed) {
			thread_destroy(test_thread);
			thread_destroyed = 1;

			_dump_threads();
		}
	}
}

void test_thread_1()
{
	kthread_log("initialised\n");
	while (1) {
		int interval = 140000000;
		while (interval--);

		kthread_log("still alive: %d\n", counter);
		counter++;
	}
}