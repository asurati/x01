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
#include <sched.h>
#include <uart.h>
#include <sdhc.h>
#include <mbox.h>

void sd_init()
{
	uint32_t v, resp[4] = {-1};

	sdhc_send_command(SDHC_CMD0, 0, NULL);

	v = 0;
	BF_SET(v, SDHC_CMD8_PATTERN, 0xaa);
	BF_SET(v, SDHC_CMD8_VHS, SDHC_CMD8_VHS_27_36);
	sdhc_send_command(SDHC_CMD8, v, resp);

	v = resp[0];
	assert(BF_GET(v, SDHC_CMD8_PATTERN) == 0xaa);
	assert(BF_GET(v, SDHC_CMD8_VHS));

	v = 0;
	sdhc_send_command(SDHC_CMD55, 0, resp);
	/* QRPI2 sends 0x120 as the card status. */
	BF_SET(v, SDHC_ACMD41_VDD_32_33, 1);
	sdhc_send_command(SDHC_ACMD41, v, resp);
	/* QRPI2 sends 0x80ffff00 as the OCR. */
	assert(BF_GET(resp[0], SDHC_ACMD41_VDD_32_33));

	sdhc_send_command(SDHC_CMD2, 0, resp);
	uart_send_num(resp[0]);
	uart_send_num(resp[1]);
	uart_send_num(resp[2]);
	uart_send_num(resp[3]);
}
