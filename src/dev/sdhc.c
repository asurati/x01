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

#define SDHC_BASE			0x300000

#define SDHC_CAP			(SDHC_BASE + 0x40)

// 52034b4
//          2              1
// 7654 321 0 9 8 76 54 321098 7 6 543210
// 0101 001 0 0 0 00 00 110100 1 0 110100
void sdhc_init()
{
	uint32_t v;
	v = readl(io_base + SDHC_CAP);
	uart_send_num(v);
}
