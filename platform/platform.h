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
 * 	Name:	platform.h
 * 	Desc:	Kernel platform interface.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <libkern/types.h>
#include <kern/vm/pmap.h>
#include <kern/vm/vm.h>

/* platform memory layout */
kern_return_t platform_get_memory (phys_addr_t *membase, phys_size_t *memsize);

/* interrupt controller */
kern_return_t platform_get_gicv3 (void);


#endif /* __platform_h__ */