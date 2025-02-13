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
 *	Name:	vm.h
 *	Desc:	Kernel Virtual Memory interface.
 */

#ifndef __KERN_VM_H__
#define __KERN_VM_H__

#include <tinylibc/stdint.h>
#include <kern/vm/vm_types.h>
#include <kern/trace/printk.h>
#include <libkern/boot.h>

#include <kern/vm/pmap.h>

/* Kernel virtual memory area bounds */
#define VM_KERNEL_MIN_ADDRESS		((vm_address_t) 0xffffffe000000000ULL)
#define VM_KERNEL_MAX_ADDRESS		((vm_address_t) 0xfffffff3ffffffffULL)

/* Memory protection types */
#define VM_PROT_NONE				((vm_prot_t) 0x0)
#define VM_PROT_READ				((vm_prot_t) 0x1)
#define VM_PROT_WRITE				((vm_prot_t) 0x2)
#define VM_PROT_EXECUTE				((vm_prot_t) 0x3)

/* Memory mapping types */
#define VM_MAP_TYPE_INVALID			((vm_map_type_t) 0x0)
#define VM_MAP_TYPE_KERNEL			((vm_map_tyoe_t) 0x1)
#define VM_MAP_TYPE_DEVICE			((vm_map_type_t) 0x2)
#define VM_MAP_TYPE_USER			((vm_map_type_t) 0x3)

/* Virtual memory system init */
extern void arm_vm_init(struct boot_args *args,
						vm_address_t membase, vm_address_t memsize);

extern void vm_configure(void);


// cleanup
extern int vm_is_address_valid(vm_address_t addr);
extern void vm_pagetable_walk_ttbr1();
extern void vm_pagetable_walk_ttbr0();

#endif /* __kern_vm_h__ */
