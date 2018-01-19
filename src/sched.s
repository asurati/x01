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

.globl _schedule
_schedule:
	push	{r0-r12, lr}
	mrs	r3, cpsr
	mov	r2, #0			@ Not a newly created thread
	push	{r2, r3}
	str	sp, [r1];

	ldr	sp, [r0];
	pop	{r2, r3}
	msr	cpsr, r3

	/* if r2 is non-zero, this is a fresh thread without the
	 * stack context necessary to return to schedule() or its
	 * callers. Perform manual switch and return to the thread's
	 * routine.
	 */
	cmp	r2, #0
	beq	has_context

	/* passes r0 has the address of the context of the next thread. */
	bl	sched_switch

	/* preempt_enable is simulated by keeping the fresh thread's
	 * preempt_depth to 0. */
	pop	{r0-r12, pc}

has_context:
	/* Discard r0 of the stack. */
	pop	{r2}
	pop	{r1-r12, pc}
