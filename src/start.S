/*
 * Copyright (c) 2018 Amol Surati
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

.globl start
start:
	/* Disable IRQs and FIQs. Enter SVC mode. */
	cpsid	if, #19

	/* Set stack. */
	ldr	sp, =stack_hi_pa

	/* Restrict ICache and DCache to 16KB each. Avoids page colouring. */
	mrc	p15, 0, r0, c1, c0, 1		@ Auxiliary Control
	orr	r0, #(1 << 6)			@ CZ bit
	mcr	p15, 0, r0, c1, c0, 1

	/* VA range [0, 0x40000000) is handled by TTBR0.
	 * VA range [0x40000000, 0x100000000) is handled by TTBR1.
	 */
	mov	r0, #2				@ N=2, PD0=PD1=0
	mcr	p15, 0, r0, c2, c0, 2		@ TTBCR

	ldr	r0, =k_pd_pa
	orr	r0, #1				@ walk is Inner Cacheable.
	mcr	p15, 0, r0, c2, c0, 0		@ TTBR0
	mcr	p15, 0, r0, c2, c0, 1		@ TTBR1

	/* Initialize domain D0 to be Client. */
	mov	r0, #1
	mcr	p15, 0, r0, c3, c0, 0		@ DACR

	bl	boot_map

	/* DSB to ensure completion of stores to the tables. */
	bl	_dsb

	mrc	p15, 0, r0, c1, c0, 0		@ Control register
	orr	r0, r0, #(1 << 0)		@ M  MMU
	orr	r0, r0, #(1 << 1)		@ A  Strict Alignment
	orr	r0, r0, #(1 << 2)		@ C  DCache
	orr	r0, r0, #(1 << 11)		@ Z  Branch Prediction
	orr	r0, r0, #(1 << 12)		@ I  ICache
	orr	r0, r0, #(1 << 13)		@ V  High Vectors
	orr	r0, r0, #(1 << 23)		@ XP Subpage AP disabled
	mcr	p15, 0, r0, c1, c0, 0

	bl	_dsb

	mcr	p15, 0, r0, c7, c5, 4		@ Flush Prefetch Buffer

	ldr	sp, =stack_hi
	ldr	pc, =kmain

sink:
	wfi
	b	sink

_dsb:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ DSB
	mov	pc, lr

