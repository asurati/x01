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

struct irq {
	irq_fn fn;
	void *data;
};

static struct irq irqs_hard[IRQ_HARD_MAX];
static struct irq irqs_soft[IRQ_SOFT_MAX];

static volatile uint32_t irq_soft_mask;

int excpt_irq_soft()
{
	int i;
	uint32_t mask;

	while (1) {
		/* The store to the global mask must not be moved
		 * into a region where the IRQs are enabled. Hence,
		 * irq_enable() must provide release semantics.
		 */
		mask = irq_soft_mask;
		if (!mask)
			break;

		irq_soft_mask = 0;

		/* Without the "memory" barrier, the compiler optimizes
		 * away the inc/dec of irq_soft_depth, since it can prove
		 * that the value remains unchanged within the function.
		 */
		irq_enable();

		/* It is being assumed that number of instructions below
		 * (before the IRQs are disabled again) is large enough to
		 * allow taking pending interrupts. Else, isb() must
		 * be inserted here to force the CPU to take any pending
		 * interrupts.
		 */

		for (i = 0; i < IRQ_SOFT_MAX && mask; ++i) {
			if ((mask & (1 << i)) == 0)
				continue;
			mask &= ~(1 << i);
			irqs_soft[i].fn(irqs_soft[i].data);
		}

		irq_disable();
		/* The load of the global mask must not be moved
		 * into a region where the IRQs are enabled. Hence,
		 * irq_disable must provide acquire semantics.
		 */
	}
	return 0;
}

int excpt_irq()
{
	int i, ret;
	ret = 0;
	for (i = 0; i < IRQ_HARD_MAX; ++i)
		ret |= irqs_hard[i].fn(irqs_hard[i].data);
	return ret;
}

void irq_init()
{
	irq_soft_mask = 0;
}

int irq_insert(enum irq_hard ih, irq_fn fn, void *data)
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

/* If the compiler determines that the raise and clear calls
 * can be optimized out (at the call site), events may be los.
 * irq_soft_mask needs to be volatile to prevent that from
 * occurring.
 *
 * Change orr/bic to add/sub, remove volatile and call
 * raise and clear passing the same value as parameter.
 * The compiler optimizes away the raise and clear.
 */
void irq_soft_raise(enum irq_soft is)
{
	irq_soft_mask |= 1 << is;
}

void irq_soft_clear(enum irq_soft is)
{
	irq_soft_mask &= ~(1 << is);
}
