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

#include <irq.h>
#include <sched.h>

/* Internal API for timer implementation. */
char		timer_is_asserted();
void		timer_rearm(uint32_t freq);
void		timer_disable();
void		timer_enable();
uint32_t	timer_freq();

/* RPi1 and RPi2 have System Timer at PA 0x20003000 and 0x3f003000,
 * respectively. A System Timer is implemented by the SoC.
 *
 * RPi2 additionally has Generic Timers, also called Core Timers,
 * which are implemented within the CPU.
 *
 * QRPI2 does not implement the System Timer.
 *
 * Hence, depending on the board selected, either the System Timer
 * or the Generic Timer must be implemented.
 */

static uint32_t freq;
static int ticks;

/* Runs with IRQs disabled. */
static int timer_irq(void *data)
{
	int ret;

	data = data;

	ret = IRQH_RET_NONE;
	if (timer_is_asserted()) {
		/* Accumulate ticks since the last run of timer's
		 * soft irq. Races with the updates made by
		 * the soft irq routine.
		 */
		++ticks;
		irq_soft_raise(IRQ_SOFT_TIMER);

		timer_rearm(freq);
		ret = IRQH_RET_HANDLED | IRQH_RET_SOFT;
	}
	return ret;
}

/* Runs with IRQs enabled. */
static int timer_irq_soft(void *data)
{
	int lt;

	data = data;

	/* These changes race with the updates made by the IRQ.
	 * Disable the IRQ (this is UP) before changing.
	 */
	irq_disable();
	lt = ticks;
	ticks = 0;
	irq_soft_clear(IRQ_SOFT_TIMER);
	irq_enable();

	sched_timer_tick(lt);
	return 0;
}

void timer_init()
{
	ticks = 0;

	timer_disable();

	irq_insert(IRQ_HARD_TIMER, timer_irq, NULL);
	irq_soft_insert(IRQ_SOFT_TIMER, timer_irq_soft, NULL);

	freq = timer_freq();
	timer_rearm(freq);
	timer_enable();
}
