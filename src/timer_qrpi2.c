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

enum timer_reg {
	TMR_FREQ,
	TMR_TVAL,
	TMR_CTRL,
	TMR_MAX
};

#define TMR_CTRL_ENABLE_POS		0
#define TMR_CTRL_MASK_POS		1
#define TMR_CTRL_STATUS_POS		2
#define TMR_CTRL_ENABLE_SZ		1
#define TMR_CTRL_MASK_SZ			1
#define TMR_CTRL_STATUS_SZ		1

static uint32_t freq;

static void timer_reg_write(enum timer_reg r, uint32_t v)
{
	assert(r < TMR_MAX);
	switch (r) {
	case TMR_CTRL:
		asm volatile("mcr	p15, 0, %0, c14, c2, 1"
			     : : "r" (v));
		break;
	case TMR_TVAL:
		asm volatile("mcr	p15, 0, %0, c14, c2, 0"
			     : : "r" (v));
		break;
	default:
		break;
	}
}

static uint32_t timer_reg_read(enum timer_reg r)
{
	uint32_t v;

	assert(r < TMR_MAX);
	switch (r) {
	case TMR_FREQ:
		asm volatile("mrc	p15, 0, %0, c14, c0, 0"
			     : "=r" (v));
		break;
	case TMR_CTRL:
		asm volatile("mrc	p15, 0, %0, c14, c2, 1"
			     : "=r" (v));
		break;
	default:
		v = -1;
		break;
	}
	return v;
}

static void timer_disable()
{
	uint32_t v;

	v = timer_reg_read(TMR_CTRL);
	v &= ~1;
	timer_reg_write(TMR_CTRL, v);
}

static void timer_enable()
{
	uint32_t v;

	v = timer_reg_read(TMR_CTRL);
	v |= 1;
	timer_reg_write(TMR_CTRL, v);
}

static int timer_irq(void *data)
{
	int ret;
	uint32_t v;

	data = data;

	ret = IRQH_RET_NONE;
	v = timer_reg_read(TMR_CTRL);
	if (BF_GET(v, TMR_CTRL_STATUS)) {
		/* Re-arm the timer. */
		timer_reg_write(TMR_TVAL, freq);
		ret = IRQH_RET_HANDLED;
	}
	return ret;
}

void timer_init()
{
	volatile uint32_t *ctl;

	timer_disable();

	irq_insert(IRQ_DEV_TIMER, timer_irq, NULL);

	/* QA7_rev3.4.
	 * Connect the cntpnsirq to the CPU IRQ interrupt. */
	ctl = (volatile uint32_t *)(ctrl_base + 0x40);
	*ctl |= 1 << 1;

	freq = timer_reg_read(TMR_FREQ);
	timer_reg_write(TMR_TVAL, freq);
	timer_enable();
}
