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
#include <atomic.h>
#include <mmu.h>
#include <pm.h>
#include <slub.h>
#include <string.h>
#include <io.h>
#include <irq.h>

struct irq {
	irq_fn fn;
	void *data;
};

static int irq_count;
static struct irq irq_devs[IRQ_DEV_MAX];
static struct atomic irq_soft;

/* irq_enter, irq_exit and excpt_irq called with IRQs disabled. */

void irq_enter()
{
	++irq_count;
	assert(irq_count > 0);
}

void irq_exit()
{
	--irq_count;
	assert(irq_count >= 0);
}

void excpt_irq()
{
	int i;
	for (i = 0; i < IRQ_DEV_MAX; ++i)
		irq_devs[i].fn(irq_devs[i].data);
}

void irq_init()
{
	irq_count = 0;
	/* Enable IRQs within CPSR. */
	asm volatile("cpsie i");
}

int irq_insert(enum irq_dev dev, irq_fn fn, void *data)
{
	assert(dev < IRQ_DEV_MAX);
	assert(irq_devs[dev].fn == NULL);

	irq_devs[dev].fn = fn;
	irq_devs[dev].data = data;

	return 0;
}

void irq_soft_raise(enum irq_dev dev)
{
	atomic_set(&irq_soft, 1 << dev);
}
