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

/* The section is mapped at hivecs through the linker script. */

.section .excpt,"ax"

excpt_vector:
	ldr	pc, excpt_reset_addr
	ldr	pc, excpt_undef_addr
	ldr	pc, excpt_svc_addr
	ldr	pc, excpt_pabort_addr
	ldr	pc, excpt_dabort_addr
	ldr	pc, excpt_res_addr
	b	excpt_irq_addr
	ldr	pc, excpt_fiq_addr

excpt_reset_addr:
	.word excpt_reset
excpt_undef_addr:
	.word excpt_undef
excpt_svc_addr:
	.word excpt_svc
excpt_pabort_addr:
	.word excpt_pabort
excpt_dabort_addr:
	.word excpt_dabort
excpt_res_addr:
	.word excpt_res
excpt_fiq_addr:
	.word excpt_fiq


/* Keep sp_irq 8-byte aligned. */
excpt_irq_addr:
	sub	lr, lr, #4		@ Adjust the return address.

	srsdb	sp!, #19		@ Save spsr_irq and lr_irq
					@ into the SVC stack
	cps	#19			@ Switch to SVC mode
	push	{r0-r3, r12, lr}	@ Save aapcs regs + lr_svc

	bl	excpt_irq

	pop	{r0-r3, r12, lr}
	rfeia	sp!			@ Undo srsdb
