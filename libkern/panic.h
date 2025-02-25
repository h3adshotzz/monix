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
 *	Name:	panic.h
 *	Desc:	Kernel panic handler.
 */

#ifndef __LIBKERN_PANIC_H__
#define __LIBKERN_PANIC_H__

#include <arch/arch.h>

extern void panic(const char *fmt, ...);
extern void panic_with_thread_state(arm64_exception_frame_t *frame, const char *fmt, ...);

#endif /* __libkern_panic_h__ */
