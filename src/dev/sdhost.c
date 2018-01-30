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

void sdhost_init()
{
	uint32_t rate;

	rate = mbox_clk_rate_get(MBOX_CLK_EMMC);
	uart_send_num(rate);
	uart_send_num(readl(io_base + SD_VDD));
}
