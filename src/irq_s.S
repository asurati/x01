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

.align 2
.globl irq_vector
irq_vector:
	ldr	pc, irq_vec_reset_addr
	ldr	pc, irq_vec_undef_addr
	ldr	pc, irq_vec_svc_addr
	ldr	pc, irq_vec_pabort_addr
	ldr	pc, irq_vec_dabort_addr
	ldr	pc, irq_vec_res_addr
	ldr	pc, irq_vec_irq_addr
	ldr	pc, irq_vec_fiq_addr

irq_vec_reset_addr:
	.long irq_vec_reset
irq_vec_undef_addr:
	.long irq_vec_undef
irq_vec_svc_addr:
	.long irq_vec_svc
irq_vec_pabort_addr:
	.long irq_vec_pabort
irq_vec_dabort_addr:
	.long irq_vec_dabort
irq_vec_res_addr:
	.long irq_vec_res
irq_vec_irq_addr:
	.long irq_vec_irq
irq_vec_fiq_addr:
	.long irq_vec_fiq
