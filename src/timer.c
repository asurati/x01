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

static int timer_irq(void *data)
{
	int ret;

	data = data;

	ret = IRQH_RET_NONE;
	if (timer_is_asserted()) {
		timer_rearm(freq);
		ret = IRQH_RET_HANDLED;
	}
	return ret;
}

void timer_init()
{
	timer_disable();

	irq_insert(IRQ_DEV_TIMER, timer_irq, NULL);

	freq = timer_freq();
	timer_rearm(freq);
	timer_enable();
}
