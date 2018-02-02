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

enum mbox_clock {
	MBOX_CLK_EMMC = 1,
	MBOX_CLK_UART,
	MBOX_CLK_MAX,
};

int		mbox_fb_alloc(const struct mbox_fb_buf *b);
uint32_t	mbox_clk_rate_get(enum mbox_clock c);
#endif
