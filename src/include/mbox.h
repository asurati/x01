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

#ifndef _MBOX_H_
#define _MBOX_H_

#include <ioreq.h>

enum mbox_property {
	MBOX_PROP_MAC,
	MBOX_PROP_MAX
};

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

struct mbox_fb_buf {
	uint32_t phy_width;
	uint32_t phy_height;
	uint32_t virt_width;
	uint32_t virt_height;
	uint32_t pitch;
	uint32_t depth;
	uint32_t virt_x;
	uint32_t virt_y;
	uint32_t addr;
	uint32_t sz;
};

#define MBOX_IOCTL_FB_ALLOC			1
#define MBOX_IOCTL_UART_CLOCK			2

int		mbox_io(struct io_req *r);
int		mbox_fb_alloc(int width, int height, int depth,
			      uintptr_t *addr, size_t *sz, int *pitch);
uint32_t	mbox_uart_clk();
#endif
