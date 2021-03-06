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

#include <assert.h>
#include <irq.h>
#include <sys/sched.h>

struct irq {
	irq_fn fn;
	void *data;
};

static struct irq irqs_hard[IRQ_HARD_MAX];
static struct irq irqs_soft[IRQ_SOFT_MAX];
static struct irq irqs_sched[IRQ_SCHED_MAX];

static uint32_t irq_soft_mask;
static uint32_t irq_sched_mask;

_ctx_sched
int irq_sched()
{
	int i;
	uint32_t mask;

	while (1) {
		/* Disabling soft IRQs is not necessary since the function
		 * is called from the primary IRQ context which ensures that
		 * no soft IRQs can run and modify the mask while the primary
		 * context itself is running.
		 */

		mask = *(volatile uint32_t *)&irq_sched_mask;
		*(volatile uint32_t *)&irq_sched_mask = 0;

		if (!mask)
			break;

		for (i = 0; i < IRQ_SCHED_MAX && mask; ++i, mask >>= 1)
			if ((mask & 1) && irqs_sched[i].fn)
				irqs_sched[i].fn(irqs_sched[i].data);

		/* When i == IRQ_SCHED_SCHEDULE, we are waking
		 * up from a schedule() which was
		 * triggered possibly due to quota consumption.
		 * Re-read the mask. The fact that
		 * IRQ_SCHED_SCHEDULE is processed in the end,
		 * no extra checks are needed to force the
		 * re-read.
		 */
	}
	return 0;
}

/* Runs with interrupts enabled, but soft IRQs disabled (to prevent
 * recursive calls). */

_ctx_soft
int irq_soft()
{
	int i;
	uint32_t mask;

	while (1) {
		irq_disable();
		mask = *(volatile uint32_t *)&irq_soft_mask;
		*(volatile uint32_t *)&irq_soft_mask = 0;
		irq_enable();

		if (!mask)
			break;

		for (i = 0; i < IRQ_SOFT_MAX && mask; ++i, mask >>= 1)
			if ((mask & 1) && irqs_soft[i].fn)
				irqs_soft[i].fn(irqs_soft[i].data);
	}
	return 0;
}

_ctx_hard
int irq_hard()
{
	int i, ret;
	ret = 0;
	for (i = 0; i < IRQ_HARD_MAX; ++i)
		if (irqs_hard[i].fn)
			ret |= irqs_hard[i].fn(irqs_hard[i].data);
	return ret;
}

void irq_init()
{
	*(volatile uint32_t *)&irq_soft_mask = 0;
	*(volatile uint32_t *)&irq_sched_mask = 0;
}

int irq_hard_insert(enum irq_hard ih, irq_fn fn, void *data)
{
	assert(ih < IRQ_HARD_MAX);
	assert(irqs_hard[ih].fn == NULL);

	irqs_hard[ih].fn = fn;
	irqs_hard[ih].data = data;

	return 0;
}

int irq_soft_insert(enum irq_soft is, irq_fn fn, void *data)
{
	assert(is < IRQ_SOFT_MAX);
	assert(irqs_soft[is].fn == NULL);

	irqs_soft[is].fn = fn;
	irqs_soft[is].data = data;

	return 0;
}

int irq_sched_insert(enum irq_sched is, irq_fn fn, void *data)
{
	assert(is < IRQ_SCHED_MAX);
	assert(irqs_sched[is].fn == NULL);

	irqs_sched[is].fn = fn;
	irqs_sched[is].data = data;

	return 0;
}

/* If the compiler determines that the raise and clear calls
 * can be optimized out (at the call site), events may be los.
 * irq_soft_mask needs to be volatile to prevent that from
 * occurring.
 *
 * Change orr/bic to add/sub, remove volatile and call
 * raise and clear passing the same value as parameter.
 * The compiler optimizes away the raise and clear.
 *
 * Implement READ_ONCE and WRITE_ONCE.
 */
_ctx_hard
void irq_soft_raise(enum irq_soft is)
{
	*(volatile uint32_t *)&irq_soft_mask |= 1 << is;
}

_ctx_hard
void irq_soft_clear(enum irq_soft is)
{
	*(volatile uint32_t *)&irq_soft_mask &= ~(1 << is);
}

_ctx_soft
void _ctx_hard irq_sched_raise(enum irq_sched is)
{
	*(volatile uint32_t *)&irq_sched_mask |= 1 << is;
}

_ctx_soft
_ctx_hard
void irq_sched_clear(enum irq_sched is)
{
	*(volatile uint32_t *)&irq_sched_mask &= ~(1 << is);
}
