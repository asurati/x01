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

.globl excpt_vector
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
	sub	lr, lr, #4		@ adjust the return address.
	push	{r0-r3, r12, lr}	@ save LR on the IRQ stack.

	bl	irq_enter		@ An IRQ is under process.
	bl	excpt_irq		@ IRQ handler.
	bl	irq_exit		@ record exit from the handler.

	/* Switch to SYS mode, enable IRQs and process soft IRQs.
	 * Then disable IRQs, switch back to IRQ mode and return.
	 */
	pop	{r0-r3, r12, lr}
	movs	pc, lr
