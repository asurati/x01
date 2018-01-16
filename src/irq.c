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

static char irq_soft_count;
static struct irq irq_devs[IRQ_DEV_MAX];
static uint32_t irq_soft_mask;

int excpt_irq_soft()
{
	/* If another instance of the function is running,
	 * return.
	 */
	if (irq_soft_count)
		return 0;

	/* Without the "memory" barrier, the compiler optimizes
	 * away the inc/dec of irq_soft_count, since it can prove
	 * that the count remains unchanged within the function.
	 */
	irq_soft_count = 1;
	asm volatile("cpsie i" : : : "memory", "cc");

	asm volatile("cpsid i" : : : "memory", "cc");
	irq_soft_count = 0;
	return 0;
}

int excpt_irq()
{
	int i, ret;
	ret = 0;
	for (i = 0; i < IRQ_DEV_MAX; ++i)
		ret |= irq_devs[i].fn(irq_devs[i].data);
	return ret;
}

void irq_init()
{
	irq_soft_count = 0;
	irq_soft_mask = 0;

	/* Enable IRQs within CPSR. */
	asm volatile("cpsie i" : : : "cc");
}

int irq_insert(enum irq_dev dev, irq_fn fn, void *data)
{
	assert(dev < IRQ_DEV_MAX);
	assert(irq_devs[dev].fn == NULL);

	irq_devs[dev].fn = fn;
	irq_devs[dev].data = data;

	return 0;
}

/* SoftIRQs are raised and dropped while the hardIRQs
 * are disabled. Atomic unncessary.
 */
void irq_soft_raise(enum irq_dev dev)
{
	irq_soft_mask |= 1 << dev;
}
