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
	IRQ_HARD_UART,
	IRQ_HARD_SDHC,
	IRQ_HARD_MBOX,
	IRQ_HARD_MAX
};

enum irq_soft {
	IRQ_SOFT_TIMER,
	IRQ_SOFT_UART,
	IRQ_SOFT_SDHC,
	IRQ_SOFT_MBOX,
	IRQ_SOFT_MAX
};

/* If a driver raises sched_timer, it may need to drop
 * any other sched_ irqs.
 */
enum irq_sched {
	IRQ_SCHED_TIMER,
	IRQ_SCHED_UART,
	IRQ_SCHED_SDHC,
	IRQ_SCHED_MBOX,
	IRQ_SCHED_SCHEDULE,		/* Should be the last. */
	IRQ_SCHED_MAX
};

/* Since IRQ enable/disable are used for synchronization,
 * rovide memory ordering.
 *
 * IRQ disable acts as an acquire barrier (or better),
 * while IRQ enable as a release barrier (or better).
 */
static inline void irq_enable()
{
	asm volatile("cpsie i" : : : "cc", "memory");
}

/* CPSID is probably a hw read/write barrier. So dmb()
 * is unnecessary. CPSIE probably does not implement
 * any type of hw barrier.
 */
static inline void irq_disable()
{
	asm volatile("cpsid i" : : : "cc", "memory");
}

#ifdef QRPI2

static inline void wfi()
{
	asm volatile("wfi");
}

#else	/* QRPI2 */

static inline void wfi()
{
	asm volatile("mcr	p15, 0, %0, c7, c0, 4"
		     : : "r" (0));
}

#endif

#define IRQH_RET_NONE		0
#define IRQH_RET_HANDLED	(1 << 0)
#define IRQH_RET_SOFT		(1 << 1)
#define IRQH_RET_SCHED		(1 << 2)

typedef int (*irq_fn)(void *data);

int	irq_hard_insert(enum irq_hard ih, irq_fn fn, void *data);
int	irq_soft_insert(enum irq_soft is, irq_fn fn, void *data);
int	irq_sched_insert(enum irq_sched is, irq_fn fn, void *data);
void	irq_soft_raise(enum irq_soft is);
void	irq_soft_clear(enum irq_soft is);
void	irq_sched_raise(enum irq_sched is);
void	irq_sched_clear(enum irq_sched is);
#endif
