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

#ifndef _SYS_MBOX_H_
#define _SYS_MBOX_H_

#include <types.h>

#define MBOX_BASE		0xb880

#define MBOX_ARM_RO		(MBOX_BASE)
#define MBOX_ARM_WO		(MBOX_BASE + 0x20)

#define MBOX_RO_IRQ_EN_POS	0
#define MBOX_RO_IRQ_PEND_POS	4
#define MBOX_RO_IRQ_EN_SZ	1
#define MBOX_RO_IRQ_PEND_SZ	1

#define MBOX_BUF_TYPE_POS	31
#define MBOX_BUF_TYPE_SZ	1

#define MBOX_CH_FB		1
#define MBOX_CH_PROP		8

#define MBOX_IOCTL_FB_ALLOC	1
#define MBOX_IOCTL_UART_CLOCK	2

struct mbox_tag {
	uint32_t id;
	uint32_t sz;
	uint32_t type;
};

struct mbox_tag_clk_rate {
	struct mbox_tag hdr;
	uint32_t id;
	uint32_t rate;
};

struct mbox_prop_buf {
	uint32_t sz;
	uint32_t code;
	union {
		struct mbox_tag_clk_rate clk_rate;
	} u;
	uint32_t end;
};

struct mbox {
	uint32_t rw;
	uint32_t r[3];
	uint32_t peek;
	uint32_t sender;
	uint32_t status;
	uint32_t config;
};
#endif
