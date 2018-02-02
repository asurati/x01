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
#include <ioreq.h>
#include <mbox.h>
#include <string.h>
#include <div.h>
#include <sdhc.h>
#include <uart.h>

#include <sys/ioreq.h>
#include <sys/sdhc.h>

/* Arasan/Broadcom specific SDHC implementation. */

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

static struct io_req_queue sdhc_ioq;

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

/* TODO place appropriate barriers. */
_ctx_sched
static void sdhc_done_ioctl(struct io_req *ior)
{
	uint32_t *p;
	struct sdhc_cmd_info *ci;

	ci = ior->io.ioctl.arg;
	p = ci->resp;

	switch (ior->io.ioctl.cmd) {
	case SDHC_IOCTL_COMMAND:
		switch (ci->cmd) {
		case SDHC_CMD2:
			p[1] = readl(io_base + SDHC_RESP1);
			p[2] = readl(io_base + SDHC_RESP2);
			p[3] = readl(io_base + SDHC_RESP3);
			/* fall through. */

		case SDHC_CMD8:
		case SDHC_CMD55:
		case SDHC_ACMD41:
			p[0] = readl(io_base + SDHC_RESP0);
			break;
		default:
			break;
		}
		break;
	default:
		assert(0);
		break;
	}
}

_ctx_sched
static int sdhc_sched_irq(void *p)
{
	struct io_req *ior;

	ior = ioq_ior_dequeue_sched(p);
	if (ior->type == IOR_TYPE_IOCTL)
		sdhc_done_ioctl(ior);
	else
		assert(0);
	ioq_ior_done_sched(p, ior);
	return 0;
}

_ctx_sched
static void sdhc_ioctl(struct io_req *ior)
{
	uint32_t v;
	struct sdhc_cmd_info *ci;

	ci = ior->io.ioctl.arg;

	switch (ior->io.ioctl.cmd) {
	case SDHC_IOCTL_COMMAND:
		writel(ci->arg, io_base + SDHC_ARG1);
		v = 0;
		BF_SET(v, SDHC_CMD_IXCHK_EN, 1);
		BF_SET(v, SDHC_CMD_CRCCHK_EN, 1);
		BF_SET(v, SDHC_CMD_INDEX, ci->cmd);

		switch (ci->cmd) {
		case SDHC_CMD8:
		case SDHC_CMD55:
		case SDHC_ACMD41:
			BF_SET(v, SDHC_CMD_RESP_TYPE, SDHC_CMD_RESP_48);
			break;
		case SDHC_CMD2:
			BF_SET(v, SDHC_CMD_RESP_TYPE, SDHC_CMD_RESP_136);
			break;
		default:
			break;
		}
		writel(v, io_base + SDHC_CMDTM);
		break;
	default:
		assert(0);
		break;
	}
}

_ctx_proc
int sdhc_send_command(enum sdhc_cmd cmd, uint32_t arg, void *resp)
{
	struct io_req ior;
	struct sdhc_cmd_info ci;

	ci.cmd = cmd;
	ci.arg = arg;
	ci.resp = resp;

	memset(&ior, 0, sizeof(ior));
	ior.type = IOR_TYPE_IOCTL;
	ior.io.ioctl.cmd = SDHC_IOCTL_COMMAND;
	ior.io.ioctl.arg = &ci;
	ioq_ior_submit(&sdhc_ioq, &ior);
	ioq_ior_wait(&ior);
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

	ioq_init(&sdhc_ioq, sdhc_ioctl, NULL);

	irq_hard_insert(IRQ_HARD_SDHC, sdhc_hard_irq, NULL);
	irq_sched_insert(IRQ_SCHED_SDHC, sdhc_sched_irq, &sdhc_ioq);


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
