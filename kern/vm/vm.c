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

#define pr_fmt(fmt)	"vm: " fmt

#include <kern/vm/vm_page.h>
#include <kern/vm/vm_map.h>
#include <kern/defaults.h>
#include <kern/vm/pmap.h>
#include <kern/vm/vm.h>
#include <kern/task.h>

#include <libkern/assert.h>
#include <libkern/boot.h>
#include <libkern/list.h>

#include <arch/proc_reg.h>

#define PRINT_PADDING(__n)	\
	do {	\
		if (__n == 0) kprintf("");	\
		else for (int c=0;c<__n;c++){kprintf("\t");}	\
	} while (0)

/* Read translation registers */
#define mmu_get_tcr()			sysreg_read(tcr_el1)
#define mmu_get_tt_base()		sysreg_read(ttbr0_el1)
#define mmu_get_tt_base_alt()	sysreg_read(ttbr1_el1)

/**
 * Initial pagetables used until arm_vm_init is called
 */
unsigned int bootstrap_pagetables[BOOTSTRAP_TABLE_SIZE]
	__attribute__((section(".data"))) __attribute__((aligned(TT_PAGE_SIZE)));

/* physical memory */
vm_address_t		memory_virt_base;
phys_addr_t			memory_phys_base;
phys_size_t			memory_phys_size;

/* kernel vm properties */
static phys_addr_t	kernel_phys_base;
static phys_size_t	kernel_phys_size;
static vm_address_t	kernel_virt_base;

/* kernel maps */
static struct pmap	kernel_pmap_ref __attribute__((section(".data")));
static pmap_t		*kernel_pmap = &kernel_pmap_ref;
static vm_map_t		*kernel_vm_map = (vm_map_t *)(&kernel_pmap_ref + sizeof(task_t));

static inline void mmu_set_tt_base(uint64_t base)
{
	__asm__ __volatile__("dsb sy\n\t"
		"msr TTBR0_EL1, %0\n\t"
		"isb sy"
		: : "r" (base) : "memory");
}

static inline void mmu_set_tt_base_alt(uint64_t base)
{
	__asm__ __volatile__("dsb sy\n\t"
		"msr TTBR1_EL1, %0\n\t"
		"isb sy"
		: : "r" (base) : "memory");
}

static inline void mmu_set_tcr(uint64_t tcr_val)
{
	__asm__ __volatile__("dsb sy\n\t"
		"msr TCR_EL1, %0\n\t"
		"isb sy"
		: : "r" (tcr_val) : "memory");
}

/**
 * vm_is_address_valid
 * 
 * Check that a given virtual address can be successfully translated to a
 * corresponding physical address
 */
kern_return_t vm_is_address_valid(vm_address_t va)
{
	/* if the result is 0, the address is not valid */
	return (mmu_translate_kvtop(va)) ? KERN_RETURN_SUCCESS : KERN_RETURN_FAIL;
}

/* return the kernel virtual map */
vm_map_t *vm_get_kernel_map()
{
	return (vm_map_t *)kernel_vm_map;
}

/* dump a debug view of a vm_map */
static void __vm_debug_dump_map(vm_map_t *map)
{
	// NOTE: THIS IS BROKEN, NEEDS FIXING
	pr_info("dumping virtual memory map from address: 0x%lx\n", map);
	kprintf("   timestamp: 0x%lx\n", map->timestamp);
	kprintf("        pmap: 0x%lx\n", &map->pmap);
	kprintf("         min: 0x%lx\n", map->min);
	kprintf("         max: 0x%lx\n", map->max);
	kprintf("alloc'd size: 0x%lx\n", map->size);
	kprintf("       flags: lock: %d\n", map->lock);
	kprintf("     entries: %d\n", map->nentries);

	vm_map_entry_t *entry;
	int idx = 0;
	list_for_each_entry(entry, &map->entries, siblings) {
		kprintf("  [%d]: 0x%lx -> 0x%lx (%d bytes)",
			idx, entry->base, entry->base + entry->size, entry->size);
		if (entry->guard_page)
			kprintf("\t- GUARD_PAGE");
		else if (entry->kernel_code)
			kprintf("\t- KERNEL_CODE");

		kprintf("\n");
		idx+=1;
	}
	kprintf("\n");
}

/* print a pagetable walk */
static void __vm_pagetable_walk(tt_table_t *table_base, int level, int padding)
{
	/**
	 * Pagetable walk:
	 *  1. loop through all entries within the table
	 *  2. decode each entry and determine whether it's a block/page, or table
	 *  3. block/page entries are printed, table entries recursively call the
	 *     __vm_pagetable_walk function again until it reaches the last table
	 */
	for (int idx = 0; idx < (TT_PAGE_SIZE / 8); idx++) {
		tt_entry_t entry = table_base[idx];
		uint8_t type = (entry & TTE_TYPE_MASK);

		/* table entry */
		if ((type == TTE_TYPE_TABLE) && (level < 3)) {
			vm_address_t table_address = ptokva(entry & TT_TABLE_MASK);
			PRINT_PADDING(padding);
			pr_cont("Level %d [%d]: Table descriptor @ 0x%lx:\n",
				level, idx, (entry & TT_TABLE_MASK));

			__vm_pagetable_walk((tt_table_t *)table_address, level+1, padding+1);
			continue;
		}

		/* block entry */
		if (type == TTE_TYPE_BLOCK) {
			vm_address_t block_address = ptokva(entry & TT_BLOCK_MASK);
			PRINT_PADDING(padding);
			pr_cont("Level %d [%d]: Block descriptor: 0x%lx (mapped to 0x%lx)\n",
				level, idx, (entry & TT_BLOCK_MASK), block_address);
			continue;
		}

		/* page entry */
		if (type == TTE_TYPE_PAGE) {
			vm_address_t page_address = (entry && TT_PAGE_MASK);
			pr_cont ("Level %d [%d]: Page Descriptor: 0x%lx\n",
				level, idx, page_address);
			continue;
		}
	}
}

/**
 * TTBR1_EL1 pagetable walk
 */
void vm_pagetable_walk_ttbr1()
{
	vm_address_t table_base;

	table_base = ptokva(mmu_get_tt_base_alt() & TTBR_BADDR_MASK);
	__vm_pagetable_walk((tt_table_t *)table_base, 1, 0);
}

/**
 * TTBR0_EL1 pagetable walk
 */
void vm_pagetable_walk_ttbr0()
{
	vm_address_t table_base;

	table_base = ptokva(mmu_get_tt_base() & TTBR_BADDR_MASK);
	__vm_pagetable_walk((tt_table_t *)table_base, 1, 0);
}

/**
 * Walk the given pagetable
 */
void vm_pagetable_walk(tt_table_t *table, int level)
{
	__vm_pagetable_walk(table, level, 0);
}

/**
 * vm_configure
 * 
 * Configure the main kernel virtual memory management interfaces. This is done
 * after arm_vm_init() has been called, and will create vm_page structures for
 * each physical page on the system, followed by creating the kernel's vm_map
 */
void vm_configure()
{
	/**
	 * create the vm_page's for the entire non-secure memory region. This does
	 * not create pages for device memory. The kernel is placed at the lowest
	 * useable physical memory address
	 */
	vm_page_bootstrap(kernel_phys_base, memory_phys_size, kernel_phys_size);

	/**
	 * create the kernel tasks vm_map just after the pmap structure. This is all
	 * (hopefully) within a single 4KB page
	 */
	vm_map_create(kernel_vm_map, (pmap_t*)&kernel_pmap, kernel_virt_base,
		VM_KERNEL_MAX_ADDRESS);
	vm_map_entry_create(kernel_vm_map, kernel_virt_base, kernel_phys_size,
		VM_ALLOC_KERNEL_CODE);
}

/**
 * arm_vm_init
 * 
 * Initialise the kernel pagetables and pmap structure. This will create initial
 * translation table entries for the kernel and boot conosle (uart). In the
 * future when the kernel is loaded as an ELF, this will map the various
 * sections.
 */
void arm_vm_init(struct boot_args *args, phys_addr_t membase,
		phys_size_t memsize)
{
	vm_address_t console_virt_base;

	assert(membase > 0 && memsize > 0);

	/* set global memory information */
	memory_phys_base = args->physbase;
	memory_phys_size = memsize;
	memory_virt_base = args->virtbase;

	/* creates the pagetables region, replacing the bootstrap pagetables */
	pmap_ptregion_create();

	/* create the kernel pagetables */
	kernel_tte = (tt_table_t *)pmap_ptregion_alloc();
	kernel_ttep = mmu_translate_kvtop((vm_address_t)kernel_tte);

	invalid_tte = (tt_table_t *)pmap_ptregion_alloc();
	invalid_ttep = mmu_translate_kvtop((vm_address_t)invalid_tte);

	/**
	 * tBoot passes two regions to map before configuring the rest of the
	 * virtual memory system: kernel and console.
	 * 
	 * we map the kernel to the same virtual address base as in the reset vector
	 * and map the console to the default peripherals virtual base address
	 */
	kernel_virt_base = memory_virt_base;
	kernel_phys_base = args->kernbase;
	kernel_phys_size = args->kernsize;

	console_virt_base = DEFAULTS_KERNEL_VM_PERIPH_BASE;

	/* directly create the translation table entries */
	pmap_tt_create_tte(kernel_tte, kernel_phys_base, kernel_virt_base,
		kernel_phys_size, PMAP_ACCESS_READWRITE);
	pmap_tt_create_tte(kernel_tte, args->uartbase, console_virt_base,
		args->uartsize, PMAP_ACCESS_READWRITE);

	/* switch the mmu to use the new translation tables */
	mmu_set_tt_base_alt(kernel_ttep & TTBR_BADDR_MASK);
	mmu_set_tt_base(invalid_ttep & TTBR_BADDR_MASK);
}
