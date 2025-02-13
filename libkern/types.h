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
 * 	Name:	types.h
 * 	Desc:	Kernel specific types.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

typedef int boolean_t;
typedef int integer_t;

/* Return types */
typedef int	kern_return_t;

#define KERN_RETURN_SUCCESS		0
#define KERN_RETURN_FAIL		1

#ifndef __STRING
#define __STRING(x) #x
#endif

#endif /* __types_h__ */
