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

#include <io.h>
#include <intc.h>

#define INTC_BASE			0xb200

#define INTC_PEND0			(INTC_BASE)
#define INTC_PEND1			(INTC_BASE + 0x4)
#define INTC_PEND2			(INTC_BASE + 0x8)
#define INTC_EN0			(INTC_BASE + 0x18)
#define INTC_EN1			(INTC_BASE + 0x10)
#define INTC_EN2			(INTC_BASE + 0x14)
#define INTC_FIQ			(INTC_BASE + 0xc)

/* STIMER is the System (SoC) Timer, unavailable on QRPI2. */
#define INTC_IRQ_STIMER_POS		0
#define INTC_IRQ_MBOX_POS		1
#define INTC_IRQ_STIMER_SZ		1
#define INTC_IRQ_MBOX_SZ		1

/* On QRPI2, this controller does not manage the Core timers. */
void intc_init()
{
	uint32_t v;

	v = 0;

	/* Disable everything. */
	writel(v, io_base + INTC_EN0);
	writel(v, io_base + INTC_EN1);
	writel(v, io_base + INTC_EN2);
	writel(v, io_base + INTC_FIQ);

	/* Enable MBOX interrupt. */
	BF_SET(v, INTC_IRQ_MBOX, 1);
	writel(v, io_base + INTC_EN0);

	/* TODO Enable STIMER interrupt on RPi1. */
}
