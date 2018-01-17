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
	ands	r0, r0, #(1 << 1)	@ IRQH_RET_SOFT
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

	/* This is the point where IRQs are disabled,
	 * all soft IRQs have been processed, IRQ depth is 1,
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

	/* No soft IRQs running at this point. Check if the current
	 * thread needs eviction.
	 */
	bl	sched_needs_eviction
	cmp	r0, #0
	beq	no_irq_soft

	/* The thread must be evicted off the CPU. */

	bl	excpt_irq_exit

	/* Clear the IRQ stack. */
	pop	{r0-r3, r12, lr}

	/* sp_svc is assumed to be 8byte aligned, and we keep it
	 * that way.
	 */
	srsdb	sp!, #19		@ Save spsr_irq and lr_irq
					@ into the SVC stack
	/* spsr_irq is pushed first, then lr_irq. Thus, lr_irq
	 * is at a lower address
	 */
	cps	#19			@ Switch to SVC mode
	push	{r0-r12}
	sub	sp, sp, #4		@ Realign
	mov	r0, sp

	/* r0 is the context frame and the stack pointer of
	 * the outgoing frame.
	 */
	bl	swtch

	/* swtch returns the pointer to the context frame of the
	 * next thread.
	 */
	mov	sp, r0
	add	sp, sp, #4

	mrs	r0, spsr
	push	{r0, lr}		@ Save SVC's registers

	/* Pull spsr_irq and lr_irq from the SVC stack and
	 * save them on the IRQ stack
	 */
	ldr	r0, [sp, #15*4]		@ lr_irq
	ldr	r1, [sp, #16*4]		@ spsr_irq
	mov	lr, r0
	msr	spsr, r1
	srsdb	sp!, #18

	pop	{r0, lr}		@ Restore SVC's registers
	msr	spsr, r0

	pop	{r0-r12}		@ Restore context
	add	sp, sp, #8		@ Pop spsr_irq and lr_irq

	cps	#18
	rfeia	sp!

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
