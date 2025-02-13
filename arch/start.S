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
 *	Name:	start.S
 *	Desc:	Kernel entry point, prepares bootstrap pagetables and jump to C
 *			code init.
 */

#include <arch/proc_reg.h>
#include <kern/defaults.h>

#include <libkern/image.h>

/*******************************************************************************
 * Name:	create_tt_table_entry
 * Desc:	Create a Table entry at the given translation table which points to
 * 			a next-level translation table. Temporary parameters are overwritten.
 *
 * 			virt_addr		-	Virtual address to create an entry for.
 * 			ln_table		-	Current translation table pointer.
 * 			lx_table		-	Next translation table pointer.
 * 			index_mask		-	Bitmask to get Ln table index for virt_addr.
 * 			shift			-	Shift for this translation table level.
 *
*******************************************************************************/
.macro create_tt_table_entry    virt_addr, ln_table, lx_table, index_mask, shift, tmp1, tmp2
	/* get the index into the Ln table from virt_addr */
	and		\tmp1, \virt_addr, #(\index_mask)
	lsr		\tmp1, \tmp1, #(\shift)

	/* convert index into pointer */
	lsl		\tmp1, \tmp1, #(TTE_SHIFT)
	add		\tmp1, \ln_table, \tmp1

	/* create the entry, encoding the address of the next Lx table */
	and		\tmp2, \lx_table, #(TT_TABLE_MASK)
	orr		\tmp2, \tmp2, #(0x3)
	str		\tmp2, [\tmp1]
.endm


/*******************************************************************************
 * Name:	create_tt_block_entries
 * Desc:	Create a number of Block entries for translating a given virtual
 * 			address into a given physical address. Temporary parameters are
 * 			overwritten.
 *
 * 			virt_addr		-	Virtual address to create block entries for.
 * 			phys_addr		-	Physical address to create block entries for.
 * 			ln_table		-	Current translation table pointer.
 * 			index_mask		-	Bitmask to get Ln table index for virt_addr.
 * 			shift			-	Shift for this translation table level.
 * 			table_mask		-	Bitmask to get address bits from phys_addr.
 * 			nents			-	Number of block entries to create in this table.
 *
*******************************************************************************/
.macro create_tt_block_entries virt_addr, phys_addr, ln_table, index_mask, shift, table_mask, nents, tmp1, tmp2, tmp3, tmp4
	/* get the index into the Ln table from the virt_addr */
	and		\tmp1, \virt_addr, #(\index_mask)
	lsr		\tmp1, \tmp1, #(\shift)

	/* convert index into pointer */
	lsl		\tmp1, \tmp1, #(TTE_SHIFT)
	add		\tmp1, \ln_table, \tmp1

	/* create an initial block entry for the given virt and phys address */
	mov		\tmp2, #(TTE_BLOCK_TEMPLATE)
	and		\tmp3, \phys_addr, #(\table_mask)
	orr		\tmp3, \tmp2, \tmp3

	mov		\tmp2, \nents
	mov		\tmp4, #(TT_L2_SIZE)

1:
	str		\tmp3, [\tmp1], #(1 << TTE_SHIFT)
	add		\tmp3, \tmp3, \tmp4
	subs	\tmp2, \tmp2, #1
	b.ne 	1b
.endm


/*******************************************************************************
 * Name:	create_bootstrap_region
 * Desc:	Create a bootstrap translation table for a given virtual address to
 * 			physical address range. This creates a two-level mapping using Block
 * 			entries. Temporary parameters are overwritten.
 *
 * 			virt_addr		-	Virtual address to create block entries for.
 * 			phys_addr		-	Physical address to create block entries for.
 * 			entries			-	Number of entries to create.
 * 			l1_table		-	Level 1 table pointer.
 * 			l2_table		-	Level 2 table pointer.
 *
*******************************************************************************/
.macro create_bootstrap_region virt_addr, phys_addr, entries, l1_table, l2_table, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6
	/* calculate number of entries remaining */
	and		\tmp1, \virt_addr, #(TT_L2_INDEX_MASK)
	lsr		\tmp1, \tmp1, #(TT_L2_SHIFT)
	mov		\tmp2, #(TTE_PAGE_ENTRIES)
	sub		\tmp1, \tmp2, \tmp1

	/* allocate a Level 2 table */
3:  add		\l2_table, \l2_table, TT_PAGE_SIZE

	/* create the Level 1 table entry */
	create_tt_table_entry	\virt_addr, \l1_table, \l2_table, TT_L1_INDEX_MASK, TT_L1_SHIFT, \tmp3, \tmp4

	cmp		\entries, \tmp1
	csel	\tmp1, \entries, \tmp1, lt

	/* create the Level 2 table entry */
	create_tt_block_entries		\virt_addr, \phys_addr, \l2_table, TT_L2_INDEX_MASK, TT_L2_SHIFT, TT_BLOCK_MASK, \tmp1, \tmp3, \tmp4, \tmp5, \tmp6

	subs	\entries, \entries, \tmp1
	b.eq	2f

	add		\phys_addr, \phys_addr, \tmp1, lsl #(TT_L2_SHIFT)
	add		\virt_addr, \virt_addr, \tmp1, lsl #(TT_L2_SHIFT)

	mov		\tmp1, #(TT_PAGE_SIZE >> 3)
	b		3b
2:
.endm

/******************************************************************************
 * Early System Reset Vector
 *
 * This is the first code to execute in the Kernel. tBoot will jump here after
 * loading the kernel, either from a disk or a memory-mapped load region. The
 * kernel will always load at EL2. tBoot expects an image_info header at the
 * base of the kernel, and will jump to the offset specified in that header.
 *
 * The first steps are to mask all interrupts and prepare the boot arguments
 * pointer, which is sent across in register x0. Additionally, this is only
 * for the Boot CPU, secondary CPUs are handled differently.
 *****************************************************************************/

	.align		12
	.section	".text"		// tmp
#if DEFAULTS_KERNEL_IMAGEINFO_HEADER
	.globl		_header
_header:

	.word	IMAGE_INFO_MAGIC
	.word	KRNL_IMAGE_ID
	.quad	0x20
	.quad	0x1802000
	.quad	0x0

#endif
	.globl		_LowResetVector
_LowResetVector:

	/* disable interrupts */
	msr		DAIFSet, #(DAIF_MASK_ALL)

	/* preserve boot args */
	mov		x27, x0

	/* jump */
	b		_ResetVector

/******************************************************************************
 * Early Exception Vectors
 *
 * During the early boot process, we use an Early Exception Vector so as to
 * preserve the register state when an exception occurs, until we can configure
 * proper exception handling which saves the register state.
 *****************************************************************************/

	.align		12
	.globl		_LowExceptionVectorBase
_LowExceptionVectorBase:
	/* EL1 SP0 */
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align 	7
	b		.

	/* EL1 SP1 */
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align 	7
	b		.
	
	/* EL0 64 */
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align 	7
	b		.
	
	/* EL0 32 */
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align 	7
	b		.

/******************************************************************************
 * Kernel Reset Vector
 *
 * This configures the exception vector base register and switches the kernel
 * from EL2 to Non-secure EL1. At this stage, the boot arguments are still in
 * x27 and haven't been touched yet.
 *****************************************************************************/

	.align		2
	.globl		_ResetVector
_ResetVector:

	/* setup low exception vector before attempting anything */
	adr		x0, _LowExceptionVectorBase
	msr		VBAR_EL1, x0

	/* initialise SCTLR_EL1 */
	mov		x0, xzr
	ldr		x1, =SCTLR_RES1_MASK
	orr		x0, x0, x1
	msr		SCTLR_EL1, x0

	/* initialise HCR_EL2 */
	mrs		x0, HCR_EL2
	orr		x0, x0, #(1 << 31)		// RW=1 EL1 exec state is AArch64
	msr		HCR_EL2, x0

	/* initialise SPSR_EL2 */
	mov		x0, xzr
	mov		x0, #0b00101			// EL1
	orr		x0, x0, #(1 << 8)		// Enable SError and External Abort
	msr		SPSR_EL2, x0

	/* initialise EL2 gicv3 system register */
	mov		x0, xzr
	orr		x0, x0, #(1 << 3)		// Enable bit
	orr		x0, x0, #(1 << 0)		// SRE bit
	msr		ICC_SRE_EL2, x0

	/* initialise ELR_EL2 */
	adr		x0, _start
	msr		ELR_EL2, x0

	eret

/******************************************************************************
 * Kernel Secondary CPU Entry Vector
 *
 * This is the entry point for secondary CPUs. This is still TODO.
 *****************************************************************************/

	.align		2
	.globl		_SecondaryResetVector
_SecondaryResetVector:

	/* Prep PSCI */ 
	ldr		w0, =0xc4000003
	mov		x1, #1
	ldr		x2, =_cpu_secondary_entry
	mov		x3, #0

	hvc		0

_cpu_secondary_entry:
	brk		#1

/******************************************************************************
 * Kernel Entry Point (EL1)
 *
 * This is where the Kernel will actually start executing. By now, it's in the
 * correct execution state and we know where the boot arguments are. This will
 * configure some initial virtual memory mappings using the virtual and
 * physical addresses passed by the bootloader, and then jump to the C entry
 * point.
 *****************************************************************************/

	.section	".text"
	.align		2
	.globl		_start
_start:

	/* setup the stack pointer */
	msr		SPSel, #0
	adr		x0, intstack_top
	mov		sp, x0

	/* callback to tinyboot */
	smc		#0

	/* read memory info from boot args */
	ldr		x25, [x27, DEFAULTS_BA_OFFSET_VIRTBASE]
	ldr		x26, [x27, DEFAULTS_BA_OFFSET_PHYSBASE]
	ldr		x28, [x27, DEFAULTS_BA_OFFSET_MEMSIZE]

	adr		x1, bootstrap_pagetables	// Level 1
	mov		x2, x1						// Level 0

	mov		x3, x26						// V=P Virt cursor
	mov		x4, x26						// V=P Phys cursor

	/* V=P mapping */
	lsr		x5, x28, #(TT_L2_SHIFT)
	create_bootstrap_region		x3, x4, x5, x1, x2, x6, x7, x8, x9, x10, x11

	/* move the L1 table after the last V=P L2 table */
	add		x1, x2, TT_PAGE_SIZE
	mov		x2, x1

	mov		x3, x25						// KVA Virt cursor
	mov		x4, x26						// KVA Phys cursor

	/* KVA mapping */
	lsr		x5, x28, #(TT_L2_SHIFT)
	create_bootstrap_region		x3, x4, x5, x1, x2, x6, x7, x8, x9, x10, x11

	/* set Translation Table Base Registers for EL1 */
	adr		x0, bootstrap_pagetables
	and		x0, x0, #(TTBR_BADDR_MASK)
	and		x1, x1, #(TTBR_BADDR_MASK)
	msr		TTBR0_EL1, x0
	msr		TTBR1_EL1, x1
	tlbi	vmalle1
	dsb		ish
	isb

	/* configure TCR_EL1 */
	mov     x0, xzr
	mov     x1, #(TCR_TG0_GRANULE_SIZE_MASK)
	orr     x0, x0, x1
	mov     x1, #(TCR_TG1_GRANULE_SIZE_MASK)
	orr     x0, x0, x1
	mov     x1, #(TCR_IPS_40BITS)
	orr     x0, x0, x1
	mov     x1, #(TCR_T0SZ_MASK)
	orr     x0, x0, x1
	mov     x1, #(TCR_T1SZ_MASK)
	orr     x0, x0, x1
	msr		TCR_EL1, x0

	/* enable the MMU */
	mrs		x0, SCTLR_EL1
	orr		x0, x0, #1
	msr		SCTLR_EL1, x0
	isb

	/* set the proper exception vector */
	adr		x0, _ExceptionVectorBase
	sub		x0, x0, x26			// offset = _ExceptionVectorBase - phys base
	add		x0, x0, x25			// vbar = offset + virt base
	msr		VBAR_EL1, x0

	/* unmask interrupts */
	msr		DAIFClr, #(DAIF_MASK_ALL)

	/* calculate the KVA for kernel_init */
	mov		x0, x27
	adr		x1, kernel_init
	sub		x1, x1, x26			// entry_offset = kernel_init - phys base
	add		x1, x1, x25			// kva = entry_offset + virt base
	mov		lr, x1				// lr = kva

	/* return to C entry point */
	ret
