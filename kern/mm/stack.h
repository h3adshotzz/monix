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
 * Name:	stack.c
 * Desc:	Thread stack allocator.
*/

#ifndef __KERN_MM_STACK_H__
#define __KERN_MM_STACK_H__

#include <kern/vm/vm_page.h>
#include <kern/thread.h>
#include <kern/trace/printk.h>

extern void stack_init();

extern void stack_alloc(thread_t *thread);
extern void stack_free(thread_t *thread);


#endif /* __kern_mm_stack_h__ */