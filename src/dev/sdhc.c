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
#include <uart.h>

#include <sys/ioreq.h>
#include <sys/sdhc.h>

/* SDHC == SD Host Controller. */

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

static struct sdhc_softc softc;

_ctx_hard
static int sdhc_hard_irq(void *p)
{
	int ret;
	uint32_t v;
	struct sdhc_softc *sc;

	sc = p;
	ret = IRQH_RET_NONE;

	v = readl(io_base + SDHC_INT);
	if (v == 0)
		return IRQH_RET_NONE;

	sc->sc_int |= v;

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
		case SDHC_CMD9:
			p[1] = readl(io_base + SDHC_RESP1);
			p[2] = readl(io_base + SDHC_RESP2);
			p[3] = readl(io_base + SDHC_RESP3);
			/* fall through. */
		case SDHC_CMD3:
		case SDHC_CMD7:
		case SDHC_CMD8:
		case SDHC_CMD17:
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
	uintptr_t sc_int;
	struct io_req *ior;
	struct sdhc_softc *sc;

	sc = p;

	/* Assumption: An IRQ is not raised for SDHC while its sched IRQ routine
	 * is running.
	 */
	sc_int = sc->sc_int;
	sc->sc_int = 0;

	if (bits_get(sc_int, SDHC_INT_CMD)) {
		ior = ioq_ior_dequeue_sched(&sc->sc_ioq);
		/* TODO Support more informative errors. */
		ior->status = 0;
		if (bits_get(sc_int, SDHC_INT_ERR))
			ior->status = -1;
		else if (ior->type == IOR_TYPE_IOCTL)
			sdhc_done_ioctl(ior);
		else
			assert(0);
		ioq_ior_done_sched(&sc->sc_ioq, ior);
	}
	return 0;
}

_ctx_sched
static void sdhc_ioctl(struct io_req *ior)
{
	uint32_t v, sz;
	struct sdhc_cmd_info *ci;

	ci = ior->io.ioctl.arg;

	switch (ior->io.ioctl.cmd) {
	case SDHC_IOCTL_COMMAND:
		writel(ci->arg, io_base + SDHC_ARG1);
		v = 0;
		if (ci->cmd != SDHC_ACMD41 && ci->cmd != SDHC_CMD2 &&
		    ci->cmd != SDHC_CMD9) {
			v |= bits_on(SDHC_CMD_IXCHK_EN);
			v |= bits_on(SDHC_CMD_CRCCHK_EN);
		}
		if (ci->cmd == SDHC_CMD17) {
			v |= bits_on(SDHC_CMD_ISDATA);
			v |= bits_on(SDHC_TM_DAT_DIR);

			sz  = bits_set(SDHC_BLKSZ, SDHC_BLK_SIZE);
			sz |= bits_set(SDHC_BLKCNT, 1);
			writel(sz, io_base + SDHC_BLKSZCNT);
		}
		v |= bits_set(SDHC_CMD_INDEX, ci->cmd);

		switch (ci->cmd) {
		case SDHC_CMD2:
		case SDHC_CMD9:
			v |= bits_set(SDHC_CMD_RESP_TYPE, SDHC_CMD_RESP_136);
			break;
		case SDHC_CMD3:
		case SDHC_CMD7:
		case SDHC_CMD8:
		case SDHC_CMD17:
		case SDHC_CMD55:
		case SDHC_ACMD41:
			v |= bits_set(SDHC_CMD_RESP_TYPE, SDHC_CMD_RESP_48);
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
	ioq_ior_submit(&softc.sc_ioq, &ior);
	ioq_ior_wait(&ior);
	return ior.status;
}

_ctx_proc
static void sdhc_go_idle_state()
{
	int ret;
	ret = sdhc_send_command(SDHC_CMD0, 0, NULL);
	assert(ret == 0);
}

_ctx_proc
static void sdhc_send_if_condition()
{
	int ret;
	uint32_t v, resp;

	resp = 0;
	v  = bits_set(SDHC_CMD8_PATTERN, 0xaa);
	v |= bits_set(SDHC_CMD8_VHS, SDHC_CMD8_VHS_27_36);
	ret = sdhc_send_command(SDHC_CMD8, v, &resp);
	assert(ret == 0);
	assert(bits_get(resp, SDHC_CMD8_PATTERN) == 0xaa);
	assert(bits_get(resp, SDHC_CMD8_VHS) & SDHC_CMD8_VHS_27_36);
}

_ctx_proc
static void sdhc_send_op_condition()
{
	int i, ret;
	uint32_t v, card_sts, resp;

	for (i = 0; i < 4; ++i) {
		card_sts = 0;
		ret = sdhc_send_command(SDHC_CMD55, 0, &card_sts);
		assert(ret == 0);

		/* QRPI2 sends 0x120 as the card status. */
		assert(card_sts & (1 << 5));
		assert(card_sts & (1 << 8));

		resp = 0;
		v  = bits_on(SDHC_OCR_VDD_32_33);
		v |= bits_on(SDHC_OCR_CS);
		ret = sdhc_send_command(SDHC_ACMD41, v, &resp);
		assert(ret == 0);
		msleep(2000);

		/* QRPI2 sends 0x80ffff00 as the OCR. */
		assert(bits_get(resp, SDHC_OCR_VDD_32_33));
		if (bits_get(resp, SDHC_OCR_BUSY))
			break;
	}
}

_ctx_proc
static void sdhc_send_cid()
{
	int ret;
	uint32_t cid[4];
	ret = sdhc_send_command(SDHC_CMD2, 0, cid);
	assert(ret == 0);
}

_ctx_proc
static void sdhc_send_rca()
{
	int ret;
	uint32_t resp;

	ret = sdhc_send_command(SDHC_CMD3, 0, &resp);
	assert(ret == 0);
	softc.sc_addr = bits_get(resp, SDHC_CMD3_RCA);
}

_ctx_proc
static void sdhc_send_csd()
{
	int ret;
	uint32_t csd[4], v;

	v = bits_set(SDHC_CMD9_RCA, softc.sc_addr);
	ret = sdhc_send_command(SDHC_CMD9, v, csd);
	assert(ret == 0);
}

_ctx_proc
static void sdhc_select_card()
{
	int ret;
	uint32_t v, resp;

	v = bits_set(SDHC_CMD7_RCA, softc.sc_addr);
	ret = sdhc_send_command(SDHC_CMD7, v, &resp);
	assert(ret == 0);
	/* Card Status is 0x700 - standby mode, ready for data. */
}

_ctx_proc
static void sdhc_read_block(void *buf)
{
	int ret, i;
	uint32_t resp;
	uint32_t *p;

	ret = sdhc_send_command(SDHC_CMD17, 0, &resp);
	assert(ret == 0);
	/* Card Status is 0x900 - transfer mode, ready for data. */

	p = buf;
	for (i = 0; i < 512 / 4; ++i)
		p[i] = readl(io_base + SDHC_DATA);
}

_ctx_proc
static void sdhc_set_sdclock(uint32_t freq)
{
	int div;
	uint32_t clk, ver, v;

	clk = softc.sc_emmc_clk;
	/* Disable the internal clock and the SDCLK. */
	writel(0, io_base + SDHC_CNTRL1);

	ver = readl(io_base + SDHC_SLOT_ISR_VER);
	ver = bits_get(ver, SDHC_VER_HC);
	if (ver == 1) {
		/* Spec 2.0. */
		for (div = 1; div <= 8; ++div)
			if ((clk >> div) <= freq)
				break;
		assert((clk >> div) <= freq);
		div = (1 << div) >> 1;
		v = bits_set(SDHC_C1_CLKF_LO, div);
	} else {
		/* Spec 3.0. */
		for (div = 2; div < 2048; div += 2)
			if (clk / div <= freq)
				break;
		assert(div < 2048);
		div >>= 1;
		v  = bits_set(SDHC_C1_CLKF_LO, div);
		v |= bits_set(SDHC_C1_CLKF_HI, div >> 8);
	}

	v |= bits_on(SDHC_C1_CLK_EN);
	writel(v, io_base + SDHC_CNTRL1);

	/* Wait for internal clock stabilization. */
	while (1) {
		msleep(2000);
		v = readl(io_base + SDHC_CNTRL1);
		if (bits_get(v, SDHC_C1_CLK_STABLE))
			break;
	}

	/* Enable clock to the SD Bus. */
	v |= bits_on(SDHC_C1_SDCLK_EN);
	writel(v, io_base + SDHC_CNTRL1);
}


/* Turn OFF Interrupts, Clocks, and Power.
 * Reset all.
 * Set the SD Bus clock frequency to 400khz, and enable the clock.
 * Set the Power to 3v3, and turn it ON.
 * Turn ON all interrupts.
 * Initialize the card if inserted.
 */

_ctx_init
void sdhc_init()
{
	uint32_t v, clk, ver;

	/* Support HC versions 2.0 or 3.0. */
	ver = readl(io_base + SDHC_SLOT_ISR_VER);
	ver = bits_get(ver, SDHC_VER_HC);
	assert(ver == 1 || ver == 2);

	memset(&softc, 0, sizeof(softc));
	ioq_init(&softc.sc_ioq, sdhc_ioctl, NULL);

	irq_hard_insert(IRQ_HARD_SDHC, sdhc_hard_irq, &softc);
	irq_sched_insert(IRQ_SCHED_SDHC, sdhc_sched_irq, &softc);

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
	v = bits_on(SDHC_C1_RESET_ALL);
	writel(v, io_base + SDHC_CNTRL1);

	/* Set the SDCLK freq to <= 400KHz. RPi implements a SDHC 3.0
	 * controller. Accordingly, the clock utilizes a 10-bit raw divisor
	 * instead of the 8-bit encoded divisor used within SDHC 2.0
	 * controllers.
	 */
	softc.sc_emmc_clk = clk = mbox_clk_rate_get(MBOX_CLK_EMMC);
	sdhc_set_sdclock(SDHC_MIN_FREQ);

	/* RPi supports 3v3. Set and enable power. */
	v  = bits_set(SDHC_C0_SDBUS_VOLT, 7);
	v |= bits_on( SDHC_C0_SDBUS_PWR);
	writel(v, io_base + SDHC_CNTRL0);

	/* Enable and unmask all interrupts. */
	writel(-1, io_base + SDHC_INT_MASK);
	writel(-1, io_base + SDHC_INT_EN);

	/* Is a card inserted. */
	if ((readl(io_base + SDHC_STATUS) & (1 << 16)) == 0)
		return;

	/* If a card is found inserted, initialize it. */
	sdhc_go_idle_state();
	sdhc_send_if_condition();
	sdhc_send_op_condition();
	sdhc_send_cid();
	sdhc_send_rca();
	sdhc_send_csd();
	sdhc_select_card();
	(void)sdhc_read_block;
}

/*
 RPI

 MDT 0x00cc
 Serial 0x04a5c87d
 Name SU08G
 OID SD SanDisk
 MID 03

 SU08G
 CID
 c87d00cc
 478004a5
 53553038
 00035344

 CSDv2.0
 tran_speed = 0x32
 dsr implemented.
 c_size = 3b37

 CSD
 800a4040    8
 003b377f   40
 325b5900   72
 00400e00  104

  ------------------
 QRPI2
 beef0062    8
 2101dead   40
 51454d55   72
 00aa5859  104

 ff926000    8
 09ffffdf   40
 5a5f59e0   72
 00002600  104
 */
#if 0
void sd_parse_csd(const uint32_t resp[4])
{
	size_t sz;
	struct sd_csd c;

	c.tran_speed = (resp[2] >> 24) & 0xff;
	c.dsr = (resp[2] >> 4) & 1;
	c.c_sz  = (resp[1] >> 22) & 0x3ff;
	c.c_sz |= (resp[2] & 3) << 10;
	c.c_sz_mul = (resp[1] >> 7) & 7;
	c.read_bl_len = (resp[2] >> 8) & 0xf;

	uart_send_num(c.tran_speed);//5a

	sz = c.c_sz + 1;
	sz *= 1 << (c.c_sz_mul + 2);
	sz *= 1 << c.read_bl_len;
	uart_send_num(sz);
}

void sd_parse_cid(const uint32_t resp[4])
{
	char pnm[16] = {0};
	struct sd_cid c;

	c.mdt	 = resp[0] & 0x0fff;
	c.psn	 = resp[0] >> 16;
	c.psn	|= (resp[1] & 0xffff) << 16;
	c.prv	 = (resp[1] >> 16) & 0xff;
	c.pnm[4] = (resp[1] >> 24) & 0xff;
	c.pnm[3] = (resp[2] >>  0) & 0xff;
	c.pnm[2] = (resp[2] >>  8) & 0xff;
	c.pnm[1] = (resp[2] >> 16) & 0xff;
	c.pnm[0] = (resp[2] >> 24) & 0xff;
	c.oid	 = resp[3] & 0xffff;
	c.mid	 = (resp[3] >> 16) & 0xff;

	memcpy(pnm, c.pnm, 5);
	pnm[5] = '\r';
	pnm[6] = '\n';

	uart_send_num(c.mid);
	uart_send_num(c.oid);
	uart_send_str(pnm);
	uart_send_num(c.prv);
	uart_send_num(c.psn);
	uart_send_num(c.mdt);
}
#endif
