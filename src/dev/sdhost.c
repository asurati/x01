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
#include <mbox.h>
#include <slub.h>
#include <string.h>
#include <list.h>
#include <ioreq.h>
#include <sched.h>
#include <uart.h>

/* Based on the Linux driver drivers/mmc/host/bcm2835.c */

#define SDH_BASE			0x202000
#define GPIO_BASE			0x200000
#define GPIO_GPFSEL4			(GPIO_BASE + 0x10)
#define GPIO_GPFSEL5			(GPIO_BASE + 0x14)

#define SD_CMD				(SDH_BASE)
#define SD_ARG				(SDH_BASE + 0x04)
#define SD_TOUT				(SDH_BASE + 0x08)
#define SD_CDIV				(SDH_BASE + 0x0c)
#define SD_RSP0				(SDH_BASE + 0x10)
#define SD_RSP1				(SDH_BASE + 0x14)
#define SD_RSP2				(SDH_BASE + 0x18)
#define SD_RSP3				(SDH_BASE + 0x1c)
#define SDH_STS				(SDH_BASE + 0x20)
#define SD_VDD				(SDH_BASE + 0x30)
#define SD_EDM				(SDH_BASE + 0x34)
#define SDH_CFG				(SDH_BASE + 0x38)
#define SDH_BCT				(SDH_BASE + 0x3c)
#define SD_DATA				(SDH_BASE + 0x40)
#define SDH_BLC				(SDH_BASE + 0x50)

/* QEMU connects the SD card to the SDHCI bus by default.
 * The GPIO function-select must be performed to switch
 * the card to the SDHost bus.
 *
 * See QEMU's sdbus_reparent_card.
 *
 * GPFSEL4 on RPi - 3f020004
 * GPFSEL5 on RPi - fff
 * EMMC clock speed on RPi1 - ee6b280
 *
 * gpio48-53 are set to function ALT3, which corresponds to Arasan
 * SHDCI EMMC. Setting it to ALT0 (4) should expose the SDHost.
 *
 *
 * https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=146095
 *
 * SD0 : Broadcom SDHOST is supported by the Linux sdhost driver :
 * bcm2835-sdhost.c
 * SD1 : SDIO (Arasan eMMC/SD) is supported by the Linux mmc driver :
 * bcm2835-mmc.c
 * Previous, the SDIO (Arasan eMMC/SD) was on SD1 GPIO 48-53/ALT3
 * With the latest kernel drivers, the Broadcom SDHOST is SDO on GPIO
 * 48-53/ALT0
 * These pins are connected to the embedded SDCard interface.
 * This enables SD1 on GPIO 34-43/ALT3 for acting with the BCM43438 WiFi module
 *
 * To avoid changing the settings on the RPi1, we will program the Arasan SDHCI
 * EMMC controller instead of the custom SDHost controller.
 *
 */
#if 0
void sdhost_select()
{
	uint32_t v;

	/* 48-53 must be set to ALT0 == 4
	 * 48-49 in reg4, rest in reg5
	 */
	uart_send_num(readl(io_base + GPIO_GPFSEL4));
	uart_send_num(readl(io_base + GPIO_GPFSEL5));
	return;

	v  = 4 << (3*8);
	v |= 4 << (3*9);
	writel(v, io_base + GPIO_GPFSEL4);
	v  = 4 << (3*0);
	v |= 4 << (3*1);
	v |= 4 << (3*2);
	v |= 4 << (3*3);
	writel(v, io_base + GPIO_GPFSEL5);
}

// lxr reset_sd
//4.7.4, 7.2.7
void sdhost_init()
{
	int div;
	uint32_t rate;
	uint16_t addr;

	// power off the sd
	// setup clock to 400khz
	// power on the sd
	// send cmd0
	sdhost_select();

	rate = mbox_clk_rate_get(MBOX_CLK_EMMC);
	uart_send_num(rate);
	return;

	writel(0, io_base + SD_VDD);
	writel(0, io_base + SD_CMD);
	writel(0, io_base + SD_ARG);
	div = rate / 400000;
	div -= 2;
	writel(div, io_base + SD_CDIV);
	writel(1, io_base + SD_VDD);

	// cmd0, cmd8
	// before sending a cmd, wait for the new flag
	// to turn off
	// if the commnd has no response, set the no respose
	// flag in the cmd
	writel(0x0, io_base + SD_ARG);
	writel(0x8400, io_base + SD_CMD);
	uart_send_num(readl(io_base + SDH_STS));
	uart_send_num(readl(io_base + SD_EDM));


	writel(readl(io_base + SDH_STS), io_base + SDH_STS);

	writel(0x1aa, io_base + SD_ARG);
	writel(0x8008, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);

	writel(0x8037, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);

	writel(1 << 20, io_base + SD_ARG);
	writel(0x8029, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);

	writel(0, io_base + SD_ARG);
	writel(0x8002, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP1);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP2);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP3);
	uart_send_num(rate);

	writel(0, io_base + SD_ARG);
	writel(0x8003, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);
	addr = rate >> 16;

	writel(addr << 16, io_base + SD_ARG);
	writel(0x8009, io_base + SD_CMD);
	rate = readl(io_base + SDH_STS);
	writel(rate, io_base + SDH_STS);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP0);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP1);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP2);
	uart_send_num(rate);
	rate  = readl(io_base + SD_RSP3);
	uart_send_num(rate);
}
#endif
