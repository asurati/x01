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
#define TMR_CTRL_MASK_SZ		1
#define TMR_CTRL_STATUS_SZ		1

static char cntpnsirq_connected;
static char enabled;

static void timer_reg_write(uint32_t v, enum timer_reg r)
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

uint32_t timer_freq()
{
	return timer_reg_read(TMR_FREQ);
}

void timer_disable()
{
	uint32_t v;

	if (enabled) {
		v = timer_reg_read(TMR_CTRL);
		v &= ~1;
		timer_reg_write(v, TMR_CTRL);
		enabled = 0;
	}
}

void timer_enable()
{
	uint32_t v;

	/* QA7_rev3.4.
	 * Connect the cntpnsirq to the CPU IRQ interrupt. */
	if (cntpnsirq_connected == 0) {
		v = readl(ctrl_base + 0x40);
		v |= 1 << 1;
		writel(v, ctrl_base + 0x40);
		cntpnsirq_connected = 1;
	}

	if (enabled == 0) {
		v = timer_reg_read(TMR_CTRL);
		v |= 1;
		timer_reg_write(v, TMR_CTRL);
		enabled = 1;
	}
}

char timer_is_asserted()
{
	uint32_t v;

	v = timer_reg_read(TMR_CTRL);
	if (BF_GET(v, TMR_CTRL_STATUS))
		return 1;
	else
		return 0;
}

void timer_rearm(uint32_t freq)
{
	timer_reg_write(freq, TMR_TVAL);
}
