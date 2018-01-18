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

#ifndef _IRQ_H_
#define _IRQ_H_

#include <barrier.h>
#include <types.h>

enum irq_hard {
	IRQ_HARD_TIMER,
	IRQ_HARD_MBOX,
	IRQ_HARD_MAX
};

enum irq_soft {
	IRQ_SOFT_TIMER,
	IRQ_SOFT_MBOX,
	IRQ_SOFT_MAX
};

/* Since IRQ enable/disable are used for synchronization,
 * rovide memory ordering.
 *
 * IRQ disable acts as an acquire barrier (or better),
 * while IRQ enable as a release barrier (or better).
 */
static inline void irq_enable()
{
	dmb();
	asm volatile("cpsie i" : : : "cc");
}

/* CPSID is probably a hw read/write barrier. So dmb()
 * is unnecessary. CPSIE probably does not implement
 * any type of hw barrier.
 */
static inline void irq_disable()
{
	asm volatile("cpsid i" : : : "cc");
	dmb();
}

#define IRQH_RET_NONE		0
#define IRQH_RET_HANDLED	(1 << 0)
#define IRQH_RET_SOFT		(1 << 1)

typedef int (*irq_fn)(void *data);

void	irq_init();
int	irq_insert(enum irq_hard ih, irq_fn fn, void *data);
int	irq_soft_insert(enum irq_soft is, irq_fn fn, void *data);
void	irq_soft_raise(enum irq_soft is);
void	irq_soft_clear(enum irq_soft is);
#endif
