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
 *  Name:	support.S
 *	Desc:	Assembly functions to complete operations that aren't suitable for
 *			inlined C code
 */

#include <arch/proc_reg.h>

/*******************************************************************************
 * Name:	mmu_translate_kvtop
 * Desc:	Translate a given Kernel Virtual Address to Physical Address using
 *			the AT instruction.
*******************************************************************************/
	.globl mmu_translate_kvtop
mmu_translate_kvtop:
	at		s1e1r, x0
	isb		sy
	mrs		x1, PAR_EL1
	tbnz	x1, #0, L__mmu_translate_kvtop_invalid
	bfm		x1, x0, #0, #11
	and		x0, x1, #(TTBR_BADDR_MASK)
	ret
L__mmu_translate_kvtop_invalid:
	mov		x0, #0
	ret


/*******************************************************************************
 * Name:	generic timer
 * Desc:	Temporary generic timer functions until the timer driver can be
 *			written.
*******************************************************************************/
	.globl		arm64_timer_init
arm64_timer_init:
	msr		CNTP_TVAL_EL0, x0		// CNTP_TVAL_EL0 = initial_timer_value
	mov		x0, #1
	msr		CNTP_CTL_EL0, x0		// Enable the timer
	ret

	.globl		arm64_timer_reset
arm64_timer_reset:
	msr		CNTP_TVAL_EL0, x0		// CNTP_TVAL_EL0 = reset_timer_value
	ret

	.globl		arm64_timer_get_current
arm64_timer_get_current:
	mrs		x0, CNTPCT_EL0
	ret
