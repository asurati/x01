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
#include <uart.h>
#include <slub.h>
#include <string.h>

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

	clk = mbox_clk_rate_get(MBOX_CLK_UART);

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
	//BF_SET(v, UART_CR_TXEN, 1);
	writel(v, io_base + UART_CR);
}

void uart_send(char c)
{
	while (readl(io_base + UART_FR) & 0x20)
		wfi();
	writel(c, io_base + UART_DR);
}

void uart_send_num(uint32_t v)
{
	int i;
	char str[9];
	const char *hd = "0123456789abcdef";

	i = 0;
	while (v) {
		str[i] = hd[v & 0xf];
		v >>= 4;
		++i;
	}
	if (i == 0) {
		uart_send('0');
	} else {
		for (i = i - 1; i >= 0; --i)
			uart_send(str[i]);
	}
	uart_send('\r');
	uart_send('\n');
}

void uart_send_str(const char *s)
{
	int i;
	for (i = 0; s[i]; ++i)
		uart_send(s[i]);
}
