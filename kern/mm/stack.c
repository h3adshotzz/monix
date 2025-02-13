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

#define pr_fmt(fmt)	"stack: " fmt

#include <kern/mm/zalloc.h>
#include <kern/mm/stack.h>
#include <kern/trace/printk.h>

zone_t	*stack_zone;
list_t	stacks;

typedef struct stack {
	vm_address_t	stack_base;
	list_node_t		siblings;
} stack_t;

void stack_init(void)
{
	stack_zone = zone_create(sizeof(stack_t),
					THREAD_COUNT_MAX * sizeof(stack_t),
					"stacks_zone");

	INIT_LIST_HEAD(&stacks);

	pr_info("stack_init complete\n");
}

void stack_alloc(thread_t *thread)
{
	stack_t *stack;
	stack = zalloc(stack_zone);
	
	stack->stack_base = vm_map_alloc(vm_get_kernel_map(),
			THREAD_STACK_DEFAULT_SIZE,
			VM_ALLOC_GUARD_FIRST | VM_ALLOC_GUARD_LAST);
	list_add_tail(&stack->siblings, &stacks);

	thread->stack_base = stack->stack_base;
	thread->stack = stack->stack_base;
}

void stack_free(thread_t *thread)
{
	stack_t *stack;

	pr_info("freeing stack: 0x%llx\n", thread->stack_base);

	list_for_each_entry(stack, &stacks, siblings) {
		if ((vm_address_t)stack == thread->stack_base) {
			zfree(stack_zone, (vm_address_t)stack);
			break;
		}
	}
}