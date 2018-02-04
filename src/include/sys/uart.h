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

#ifndef _SYS_UART_H_
#define _SYS_UART_H_

#define UART_BASE		0x201000

#define UART_DR			(UART_BASE)
#define UART_FR			(UART_BASE + 0x18)
#define UART_IBRD		(UART_BASE + 0x24)
#define UART_FBRD		(UART_BASE + 0x28)
#define UART_LCRH		(UART_BASE + 0x2c)
#define UART_CR			(UART_BASE + 0x30)
#define UART_IFLS		(UART_BASE + 0x34)
#define UART_IMSC		(UART_BASE + 0x38)
#define UART_RIS		(UART_BASE + 0x3c)
#define UART_MIS		(UART_BASE + 0x40)
#define UART_ICR		(UART_BASE + 0x44)

#define UART_CR_EN_POS				 0
#define UART_CR_EN_SZ				 1
#define UART_CR_TXEN_POS			 8
#define UART_CR_TXEN_SZ				 1

#define UART_LCRH_FEN_POS			 4
#define UART_LCRH_FEN_SZ			 1
#define UART_LCRH_WLEN_POS			 5
#define UART_LCRH_WLEN_SZ			 2

#define UART_IMSC_TXIM_POS			 5
#define UART_IMSC_TXIM_SZ			 1

#define UART_LCRH_WLEN_8			 3

#define UART_TXF_LEN				16

struct uart_softc {
	uint32_t sc_uart_clk;
};

#endif
