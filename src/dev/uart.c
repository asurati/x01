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

#include <sys/uart.h>

static struct uart_softc softc;

_ctx_hard
static int uart_hard_irq(void *p)
{
	uint32_t v;
	(void)p;
	v = readl(io_base + UART_RIS);
	if (v)
		writel(v, io_base + UART_ICR);
	return 0;
}

void uart_init()
{
	uint32_t clk, ibrd, fbrd, dvdr, v;

	memset(&softc, 0, sizeof(softc));

	irq_hard_insert(IRQ_HARD_UART, uart_hard_irq, &softc);

	softc.sc_uart_clk = clk = mbox_clk_rate_get(MBOX_CLK_UART);
	//softc.sc_uart_clk = clk = 0x2dc6c0;

	dvdr = 115200 << 4;
	ibrd = clk / dvdr;
	fbrd = clk - (ibrd * dvdr);
	fbrd = (fbrd << 6) / dvdr;

	writel(0, io_base + UART_CR);
	writel(0, io_base + UART_LCRH);

	writel(ibrd, io_base + UART_IBRD);
	writel(fbrd, io_base + UART_FBRD);

	v = bits_set(UART_LCRH_WLEN, UART_LCRH_WLEN_8);
	writel(v, io_base + UART_LCRH);

	/* Enable tx interrupt on RPi.
	 * Avoid enabling on QRPi2, to prevent -d int from filling
	 * the output.
	 */
#ifdef RPI
	v = bits_set(UART_IMSC_TXIM);
	writel(v, io_base + UART_IMSC);
#endif

	v = bits_on(UART_CR_EN);
	v |= bits_on(UART_CR_TXEN);
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
