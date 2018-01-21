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
#include <mbox.h>
#include <uart.h>

#define UART_BASE		0x201000

#define UART_DR			(UART_BASE)
#define UART_FR			(UART_BASE + 0x18)
#define UART_IBRD		(UART_BASE + 0x24)
#define UART_FBRD		(UART_BASE + 0x28)
#define UART_LCRH		(UART_BASE + 0x2c)
#define UART_CR			(UART_BASE + 0x30)
#define UART_IMSC		(UART_BASE + 0x38)

#define UART_CR_EN_POS		0
#define UART_CR_EN_SZ		1
#define UART_CR_TXEN_POS	8
#define UART_CR_TXEN_SZ		1

#define UART_LCRH_FEN_POS	4
#define UART_LCRH_FEN_SZ	1
#define UART_LCRH_WLEN_POS	5
#define UART_LCRH_WLEN_SZ	2

#define UART_LCRH_WLEN_8	3

void uart_init()
{
	uint32_t clk, ibrd, fbrd, dvdr, v;

	clk = mbox_uart_clk();

	dvdr = 115200 << 4;
	ibrd = clk / dvdr;
	fbrd = clk - (ibrd * dvdr);
	fbrd = (fbrd << 6) / dvdr;

	writel(0, io_base + UART_CR);
	writel(0, io_base + UART_LCRH);
	writel(0, io_base + UART_IMSC);

	writel(ibrd, io_base + UART_IBRD);
	writel(fbrd, io_base + UART_IBRD);

	v = 0;
	BF_SET(v, UART_LCRH_FEN, 1);
	BF_SET(v, UART_LCRH_WLEN, UART_LCRH_WLEN_8);
	writel(v, io_base + UART_LCRH);

	v = 0;
	BF_SET(v, UART_CR_EN, 1);
	BF_SET(v, UART_CR_TXEN, 1);
	writel(v, io_base + UART_CR);
}

void uart_send(char c)
{
	writel(c, io_base + UART_DR);
	writel('\r', io_base + UART_DR);
	writel('\n', io_base + UART_DR);
}
