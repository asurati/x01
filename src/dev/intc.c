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

#define INTC_BASE			0xb200

#define INTC_PEND0			(INTC_BASE)
#define INTC_PEND1			(INTC_BASE + 0x4)
#define INTC_PEND2			(INTC_BASE + 0x8)
#define INTC_EN_BASIC			(INTC_BASE + 0x18)
#define INTC_EN1			(INTC_BASE + 0x10)
#define INTC_EN2			(INTC_BASE + 0x14)
#define INTC_FIQ			(INTC_BASE + 0xc)

/* STIMER is the System (SoC) Timer, unavailable on QRPI2. */

/* The RPi timer being used is the INTERRUPT_TIMER3 (C3)
 * timer, whose IRQ is at bit 3 in the EN1 register.
 *
 * The RPi Mailbox IRQ is at bit 1 in the Basic
 * (EN_BASIC) register.
 *
 */

/*
 * sdhci: sdhci {
 *	compatible = "brcm,bcm2835-sdhci";
 *	reg = <0x7e300000 0x100>;
 *	interrupts = <2 30>;
 *	clocks = <&clk_mmc>;
 *	bus-width = <4>;
 * };
 *
 * SDHCI interrupts is line# 62 = 32 + 30 = bit 30 of EN2
 */

#define INTC_IRQ_STIMER_POS		 3
#define INTC_IRQ_MBOX_POS		 1
#define INTC_IRQ_SDHC_POS		30

#define INTC_IRQ_STIMER_SZ		 1
#define INTC_IRQ_MBOX_SZ		 1
#define INTC_IRQ_SDHC_SZ		 1

/* On QRPI2, this controller does not manage the Core timers. */
void intc_init()
{
	uint32_t v;

	/* Disable FIQ. */
	writel(0, io_base + INTC_FIQ);

	/* Enable MBOX interrupt. */
	v = readl(io_base + INTC_EN_BASIC);
	BF_SET(v, INTC_IRQ_MBOX, 1);
	writel(v, io_base + INTC_EN_BASIC);

	v = readl(io_base + INTC_EN2);
	BF_SET(v, INTC_IRQ_SDHC, 1);
	writel(v, io_base + INTC_EN2);

#ifdef RPI
	/* Enable RPi SoC timer. */
	v = readl(io_base + INTC_EN1);
	BF_SET(v, INTC_IRQ_STIMER, 1);
	writel(v, io_base + INTC_EN1);
#endif
}
