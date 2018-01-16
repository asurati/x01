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
	.long excpt_reset
excpt_undef_addr:
	.long excpt_undef
excpt_svc_addr:
	.long excpt_svc
excpt_pabort_addr:
	.long excpt_pabort
excpt_dabort_addr:
	.long excpt_dabort
excpt_res_addr:
	.long excpt_res
excpt_fiq_addr:
	.long excpt_fiq

excpt_irq_addr:
	sub	lr, lr, #4		@ Adjust the return address.
	push	{r0-r3, r12, lr}	@ Save aapcs regs and lr.
	bl	excpt_irq		@ IRQ handler.
	and	r0, r0, #(1 << 1)	@ IRQH_RET_SOFT
	beq	no_irq_soft

	mrs	r0, spsr
	push	{r0}			@ Save spsr_irq

	cps	#31			@ Switch to SYS mode.
	and 	r0, sp, #4		@ Align sp_sys to 8-byte boundary.
	sub	sp, sp, r0
	push	{r0, lr}		@ Save the alignment and lr_usr.

	bl	excpt_irq_soft

	pop	{r0, lr}		@ Undo
	add	sp, sp, r0
	cps	#18			@ Back to IRQ mode

	pop	{r0}
	msr	spsr, r0

no_irq_soft:
	pop	{r0-r3, r12, lr}
	movs	pc, lr
