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
#include <io.h>
#include <irq.h>
#include <timer.h>

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

static int timer_irq(void *data)
{
	uint32_t v;
	data = data;
	asm volatile("mrc	p15, 0, %0, c14, c2, 1"
		     : "=r" (v));
	v &= ~1;
	asm volatile("mcr	p15, 0, %0, c14, c2, 1"
		     : : "r" (v));
	return 0;
}

#ifdef QRPI2
void timer_init()
{
	uint32_t cntdn;
	volatile uint32_t *ctl;

	irq_insert(IRQ_DEV_TIMER, timer_irq, NULL);

	/* QA7_rev3.4.
	 * Connect the cntpnsirq to the CPU IRQ interrupt. */
	ctl = (volatile uint32_t *)(ctrl_base + 0x40);
	*ctl |= 1 << 1;

	cntdn = 62500000;

	/* cntp_tval. */
	asm volatile("mcr	p15, 0, %0, c14, c2, 0"
		     : : "r" (cntdn));

	/* cntp_ctl. */
	asm volatile("mrc	p15, 0, %0, c14, c2, 1"
		     : "=r" (cntdn));
	cntdn |= 1;
	asm volatile("mcr	p15, 0, %0, c14, c2, 1"
		     : : "r" (cntdn));
}
#else
void timer_init()
{
	irq_insert(IRQ_DEV_TIMER, timer_irq, NULL);
}
#endif
