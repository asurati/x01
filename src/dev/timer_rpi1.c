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

#define TMR_BASE		0x3000

#define TMR_CS			(TMR_BASE)
#define TMR_CLO			(TMR_BASE + 0x4)
#define TMR_CHI			(TMR_BASE + 0x8)
#define TMR_C3			(TMR_BASE + 0x18)

#define TMR_CS_M3_POS		3
#define TMR_CS_M3_SZ		1

/* System Timer runs at 1MHz. */
uint32_t timer_freq()
{
	return 1000000;
}

/* The timer is free running. To disable it, disable the IRQ,
 * and zero the comparator.
 */
void timer_disable()
{
	writel(0, io_base + TMR_C3);
}

void timer_enable()
{
}

char timer_is_asserted()
{
	uint32_t v;
	v = readl(io_base + TMR_CS);
	if (BF_GET(v, TMR_CS_M3)) {
		/* Deassert the signal. */
		v = 0;
		BF_SET(v, TMR_CS_M3, 1);
		writel(v, io_base + TMR_CS);
		return 1;
	} else {
		return 0;
	}
}

void timer_rearm(uint32_t freq)
{
	uint32_t v;
	v = readl(io_base + TMR_CLO);
	v += freq;
	writel(v, io_base + TMR_C3);
}
