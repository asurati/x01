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
	push	{r0-r3, r12, lr}	@ Save aapcs regs and lr.

	bl	excpt_irq_enter

	bl	excpt_irq		@ IRQ handler.

	/* If the IRQ depth is > 1, do not process soft irqs here. */
	ldr	r1, =irq_depth
	ldr	r1, [r1]
	cmp	r1, #1
	bgt	no_irq_soft

	/* The IRQ depth is 1. If the handlers raised soft irqs,
	 * process them.
	 */
	and	r0, r0, #(1 << 1)	@ IRQH_RET_SOFT
	beq	no_irq_soft

	mrs	r0, spsr
	push	{r0}			@ Save spsr_irq
	sub	sp, sp, #4		@ Realign

	/* sp_sys is assumed to be 8-byte aligned.*/
	cps	#31			@ Switch to SYS mode.
	push	{lr}			@ Save lr_usr.
	sub	sp, sp, #4		@ Realign

	bl	excpt_irq_soft

	add	sp, sp, #4		@ Undo
	pop	{lr}
	cps	#18			@ Back to IRQ mode

	add	sp, sp, #4
	pop	{r0}
	msr	spsr, r0

	bl	excpt_irq_exit

	/* This is the point where IRQs are disabled,
	 * all soft IRQs have been processed, IRQ depth is 0,
	 * and IRQ stack contains r0-r3, r12 and lr.
	 *
	 * Switch threads here, if required.
	 *
	 * srsdb the spsr_irq and lr_irq registers into sp_svc.
	 * Change the mode to SVC.
	 * Push all registers.
	 * Change the sp_svc to the next task.
	 * Pop all registers.
	 * Change the mode to IRQ.
	 * Load spsr_irq and lr_irq.
	 * Return from IRQ.
	 */
	pop	{r0-r3, r12, lr}
	movs	pc, lr

no_irq_soft:
	bl	excpt_irq_exit
	pop	{r0-r3, r12, lr}
	movs	pc, lr

excpt_irq_enter:
	ldr	r0, =irq_depth		@ Increment the IRQ depth
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]
	bx	lr

excpt_irq_exit:
	ldr	r0, =irq_depth		@ Decrement the IRQ depth
	ldr	r1, [r0]
	sub	r1, r1, #1
	str	r1, [r0]
	bx	lr

.section .bss
irq_depth:	.word 0
