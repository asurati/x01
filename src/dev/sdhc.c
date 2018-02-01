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
#include <mbox.h>
#include <slub.h>
#include <string.h>
#include <list.h>
#include <ioreq.h>
#include <sched.h>
#include <uart.h>
#include <div.h>

/* Arasan/Broadcom specific SDHC implementation. */

#define SDHC_BASE				0x300000

#define SDHC_ARG2				(SDHC_BASE)
#define SDHC_BLKSZCNT				(SDHC_BASE + 0x04)
#define SDHC_ARG1				(SDHC_BASE + 0x08)
#define SDHC_CMDTM				(SDHC_BASE + 0x0c)
#define SDHC_RESP0				(SDHC_BASE + 0x10)
#define SDHC_RESP1				(SDHC_BASE + 0x14)
#define SDHC_RESP2				(SDHC_BASE + 0x18)
#define SDHC_RESP3				(SDHC_BASE + 0x1c)
#define SDHC_DATA				(SDHC_BASE + 0x20)
#define SDHC_STATUS				(SDHC_BASE + 0x24)
#define SDHC_CNTRL0				(SDHC_BASE + 0x28)
#define SDHC_CNTRL1				(SDHC_BASE + 0x2c)
#define SDHC_INT				(SDHC_BASE + 0x30)
#define SDHC_INT_MASK				(SDHC_BASE + 0x34)
#define SDHC_INT_EN				(SDHC_BASE + 0x38)
#define SDHC_CNTRL2				(SDHC_BASE + 0x3c)
#define SDHC_CAP				(SDHC_BASE + 0x40)
#define SDHC_SLOT_ISR_VER			(SDHC_BASE + 0xfc)

#define SDHC_BLKSZ_POS				 0
#define SDHC_BLKCNT_POS				16

#define SDHC_BLKSZ_SZ				10
#define SDHC_BLKCNT_SZ				16

#define SDHC_TM_BLKCNT_EN_POS			 1
#define SDHC_TM_AUTO_CMD_EN_POS			 2
#define SDHC_TM_DAT_DIR_POS			 4
#define SDHC_TM_MULTI_BLK_POS			 5
#define SDHC_CMD_RESP_TYPE_POS			16
#define SDHC_CMD_CRCCHK_EN_POS			19
#define SDHC_CMD_IXCHK_EN_POS			20
#define SDHC_CMD_ISDATA_POS			21
#define SDHC_CMD_TYPE_POS			22
#define SDHC_CMD_INDEX_POS			24

#define SDHC_TM_BLKCNT_EN_SZ			 1
#define SDHC_TM_AUTO_CMD_EN_SZ			 2
#define SDHC_TM_DAT_DIR_SZ			 1
#define SDHC_TM_MULTI_BLK_SZ			 1
#define SDHC_CMD_RESP_TYPE_SZ			 2
#define SDHC_CMD_CRCCHK_EN_SZ			 1
#define SDHC_CMD_IXCHK_EN_SZ			 1
#define SDHC_CMD_ISDATA_SZ			 1
#define SDHC_CMD_TYPE_SZ			 2
#define SDHC_CMD_INDEX_SZ			 6

#define SDHC_STS_CMD_INHB_POS			 0
#define SDHC_STS_DAT_INHB_POS			 1
#define SDHC_STS_DAT_ACT_POS			 2
#define SDHC_STS_WRDY_POS			 8
#define SDHC_STS_RRDY_POS			 9
#define SDHC_STS_CARD_IN_POS			16
#define SDHC_STS_CARD_STABLE_POS		17
#define SDHC_STS_SDCD_INV_POS			18
#define SDHC_STS_SDWP_POS			19
#define SDHC_STS_DAT_LEVEL_POS			20
#define SDHC_STS_CMD_LEVEL_POS			24
#define SDHC_STS_DAT_LEVEL1_POS			25

#define SDHC_STS_CMD_INHB_SZ			 1
#define SDHC_STS_DAT_INHB_SZ			 1
#define SDHC_STS_DAT_ACT_SZ			 1
#define SDHC_STS_WRDY_SZ			 1
#define SDHC_STS_RRDY_SZ			 1
#define SDHC_STS_DAT_LEVEL_SZ			 4
#define SDHC_STS_CMD_LEVEL_SZ			 1
#define SDHC_STS_DAT_LEVEL1_SZ			 4

#define SDHC_C0_4BIT_POS			 1
#define SDHC_C0_HS_EN_POS			 2
#define SDHC_C0_8BIT_POS			 5
#define SDHC_C0_SDBUS_PWR_POS			 8
#define SDHC_C0_SDBUS_VOLT_POS			 9

#define SDHC_C0_4BIT_SZ				 1
#define SDHC_C0_HS_EN_SZ			 1
#define SDHC_C0_8BIT_SZ				 1
#define SDHC_C0_SDBUS_PWR_SZ			 1
#define SDHC_C0_SDBUS_VOLT_SZ			 3

#define SDHC_C1_CLK_EN_POS			 0
#define SDHC_C1_CLK_STABLE_POS			 1
#define SDHC_C1_SDCLK_EN_POS			 2
#define SDHC_C1_CLK_GENSEL_POS			 5
#define SDHC_C1_CLKF_HI_POS			 6
#define SDHC_C1_CLKF_LO_POS			 8
#define SDHC_C1_DATA_TOUNIT_POS			16
#define SDHC_C1_RESET_ALL_POS			24
#define SDHC_C1_RESET_CMD_POS			25
#define SDHC_C1_RESET_DATA_POS			26

#define SDHC_C1_CLK_EN_SZ			 1
#define SDHC_C1_CLK_STABLE_SZ			 1
#define SDHC_C1_SDCLK_EN_SZ			 1
#define SDHC_C1_CLK_GENSEL_SZ			 1
#define SDHC_C1_CLKF_HI_SZ			 2
#define SDHC_C1_CLKF_LO_SZ			 8
#define SDHC_C1_DATA_TOUNIT_SZ			 4
#define SDHC_C1_RESET_ALL_SZ			 1
#define SDHC_C1_RESET_CMD_SZ			 1
#define SDHC_C1_RESET_DATA_SZ			 1

#define SDHC_VER_HC_POS				16
#define SDHC_VER_HC_SZ				 8

#define SDHC_MIN_FREQ			    400000

/*
 * From RPi
 * :SDHC_ARG2     0
 * :SDHC_BLKSZCNT 7200
 * :SDHC_ARG1     0
 * :SDHC_CMDTM    c1b0032
 * :SDHC_RESP0    b00
 * :SDHC_RESP1    0
 * :SDHC_RESP2    0
 * :SDHC_RESP3    0
 * :SDHC_DATA     0
 * :SDHC_STATUS   1ff0000
 * :SDHC_CNTRL0   f02
 * :SDHC_CNTRL1   e0307
 * :SDHC_INT      0
 * :SDHC_INT_MASK 27f003b
 * :SDHC_INT_EN   0
 * :SDHC_CNTRL2   0
 * :SDHC_CAP      0
 */

_ctx_hard
static int sdhc_hard_irq(void *data)
{
	int ret;
	uint32_t v;

	(void)data;
	ret = IRQH_RET_NONE;

	v = readl(io_base + SDHC_INT);
	if (v == 0)
		return IRQH_RET_NONE;

	writel(v, io_base + SDHC_INT);
	irq_sched_raise(IRQ_SCHED_SDHC);
	ret = IRQH_RET_HANDLED | IRQH_RET_SCHED;
	return ret;
}

_ctx_sched
static int sdhc_sched_irq(void *data)
{
	(void)data;
	return 0;
}

/* Turn OFF Interrupts, Clocks, and Power.
 * Reset all.
 * Set the SD Bus clock frequency to 400khz, and enable the clock.
 * Set the Power to 3v3, and turn it ON.
 * Turn ON all interrupts.
 */

_ctx_init
void sdhc_init()
{
	int div;
	uint32_t v, clk, ver;

	/* Support HC versions 2.0 or 3.0. */
	ver = readl(io_base + SDHC_SLOT_ISR_VER);
	ver = BF_GET(ver, SDHC_VER_HC);
	assert(ver == 1 || ver == 2);

	irq_hard_insert(IRQ_HARD_SDHC, sdhc_hard_irq, NULL);
	irq_sched_insert(IRQ_SCHED_SDHC, sdhc_sched_irq, NULL);


	/* Turn off power to the SD Bus. The controller does not expose the
	 * typical power control register. The corresponding bits are marked
	 * reserved.
	 *
	 * The below write also disables HS, and 4-bit and 8-bit data lines.
	 */
	writel(0, io_base + SDHC_CNTRL0);

	/* Disable the internal clock and the SDCLK. */
	writel(0, io_base + SDHC_CNTRL1);

	/* Unlink all interrupt status bits from the interrupt line. */
	writel(0, io_base + SDHC_INT_EN);

	/* Mask all interrupt status bits. */
	writel(0, io_base + SDHC_INT_MASK);

	/* Clear any pending interrupt status bits. */
	writel(-1, io_base + SDHC_INT);

	/* Reset all. */
	v = 0;
	BF_SET(v, SDHC_C1_RESET_ALL, 1);
	writel(v, io_base + SDHC_CNTRL1);

	/* Set the SDCLK freq to <= 400KHz. RPi implements a SDHC 3.0
	 * controller. Accordingly, the clock utilizes a 10-bit raw divisor
	 * instead of the 8-bit encoded divisor used within SDHC 2.0
	 * controllers.
	 */
	v = 0;
	clk = mbox_clk_rate_get(MBOX_CLK_EMMC);
	if (ver == 1) {
		/* Spec 2.0. */
		for (div = 1; div <= 8; ++div)
			if ((clk >> div) <= SDHC_MIN_FREQ)
				break;
		assert((clk >> div) <= SDHC_MIN_FREQ);
		div = (1 << div) >> 1;
		BF_SET(v, SDHC_C1_CLKF_LO, div);
	} else {
		/* Spec 3.0. */
		for (div = 2; div < 2048; div += 2)
			if (clk / div <= SDHC_MIN_FREQ)
				break;
		assert(clk / div <= SDHC_MIN_FREQ);
		div >>= 1;
		BF_SET(v, SDHC_C1_CLKF_LO, div);
		BF_SET(v, SDHC_C1_CLKF_HI, div >> 8);
	}

	BF_SET(v, SDHC_C1_CLK_EN, 1);
	writel(v, io_base + SDHC_CNTRL1);

	/* Wait for internal clock stabilization. */
	while (1) {
		v = readl(io_base + SDHC_CNTRL1);
		if (BF_GET(v, SDHC_C1_CLK_STABLE))
			break;
	}

	/* Enable clock to the SD Bus. */
	BF_SET(v, SDHC_C1_SDCLK_EN, 1);
	writel(v, io_base + SDHC_CNTRL1);

	/* RPi supports 3v3. Set and enable power. */
	v = 0;
	BF_SET(v, SDHC_C0_SDBUS_VOLT, 7);
	BF_SET(v, SDHC_C0_SDBUS_PWR, 1);
	writel(v, io_base + SDHC_CNTRL0);

	/* Enable and unmask all interrupts. */
	writel(-1, io_base + SDHC_INT_MASK);
	writel(-1, io_base + SDHC_INT_EN);
}
