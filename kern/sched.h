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
 * Name:	sched.h
 * Desc:	Task schedular
*/

#ifndef __KERN_SCHED_H__
#define __KERN_SCHED_H__

#include <kern/thread.h>
#include <kern/trace/printk.h>

#include <arch/arch.h>

extern uint64_t	__fork64_exec();
extern void __fork64_return();

extern void __schedule(arm64_exception_frame_t *frame);


#endif /* __kern_sched_h__ */