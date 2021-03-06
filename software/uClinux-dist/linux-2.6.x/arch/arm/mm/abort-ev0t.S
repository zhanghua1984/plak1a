/*
 *  linux/arch/arm/mm/abort-ev0t.S
 *
 *  Copyright (C) 2004 Tobias Lorenz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
/*
 * Function: v0t_early_abort
 *
 * Params  : r2 = address of aborted instruction
 *         : r3 = saved SPSR
 *
 * Returns : r0 = address of abort
 *	   : r1 = FSR, bit 11 = write
 *	   : r2-r8 = corrupted
 *	   : r9 = preserved
 *	   : sp = pointer to registers
 *
 * Purpose : obtain information about current aborted instruction.
 * Note: we read user space.  This means we might cause a data
 * abort here if the I-TLB and D-TLB aren't seeing the same
 * picture.  Unfortunately, this does happen.  We live with it.
 */
	.align	5
ENTRY(v0t_early_abort)
	tst	r3, #PSR_T_BIT
	ldrneh	r3, [r2]			@ read aborted thumb instruction
	ldreq	r3, [r2]			@ read aborted ARM instruction
	bic	r1, r1, #1 << 11 | 1 << 10	@ clear bits 11 and 10 of FSR
	movne	r3, r3, lsl #(21 - 12)		@ move thumb bit 11 to ARM bit 20
	tst	r3, #1 << 20			@ check write
	orreq	r1, r1, #1 << 11
	mov	pc, lr
