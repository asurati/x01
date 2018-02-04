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
#include <mmu.h>
#include <string.h>

/*
 *
beef0062    8
2101dead   40
51454d55   72
00aa5859  104

ff926000    8
09ffffdf   40
5a5f59e0   72
00002600  104

*/

struct sd_cid {
	uint8_t mid;
	uint16_t oid;
	uint8_t pnm[5];
	uint8_t prv;
	uint32_t psn;
	uint16_t mdt;
};

struct sd_csd {
	uint8_t tran_speed;
	uint8_t dsr;
	uint16_t c_sz;
	uint8_t c_sz_mul;
	uint8_t read_bl_len;
};

void sd_parse_csd(const uint32_t resp[4])
{
	size_t sz;
	struct sd_csd c;

	c.tran_speed = (resp[2] >> 24) & 0xff;
	c.dsr = (resp[2] >> 4) & 1;
	c.c_sz  = (resp[1] >> 22) & 0x3ff;
	c.c_sz |= (resp[2] & 3) << 10;
	c.c_sz_mul = (resp[1] >> 7) & 7;
	c.read_bl_len = (resp[2] >> 8) & 0xf;

	uart_send_num(c.tran_speed);//5a

	sz = c.c_sz + 1;
	sz *= 1 << (c.c_sz_mul + 2);
	sz *= 1 << c.read_bl_len;
	uart_send_num(sz);
}

void sd_parse_cid(const uint32_t resp[4])
{
	char pnm[16] = {0};
	struct sd_cid c;

	c.mdt	 = resp[0] & 0x0fff;
	c.psn	 = resp[0] >> 16;
	c.psn	|= (resp[1] & 0xffff) << 16;
	c.prv	 = (resp[1] >> 16) & 0xff;
	c.pnm[4] = (resp[1] >> 24) & 0xff;
	c.pnm[3] = (resp[2] >>  0) & 0xff;
	c.pnm[2] = (resp[2] >>  8) & 0xff;
	c.pnm[1] = (resp[2] >> 16) & 0xff;
	c.pnm[0] = (resp[2] >> 24) & 0xff;
	c.oid	 = resp[3] & 0xffff;
	c.mid	 = (resp[3] >> 16) & 0xff;

	memcpy(pnm, c.pnm, 5);
	pnm[5] = '\r';
	pnm[6] = '\n';

	uart_send_num(c.mid);
	uart_send_num(c.oid);
	uart_send_str(pnm);
	uart_send_num(c.prv);
	uart_send_num(c.psn);
	uart_send_num(c.mdt);
}

void sd_init()
{
	uint32_t v, resp[4] = {-1}, addr;

	/* Go Idle State. */
	sdhc_send_command(SDHC_CMD0, 0, NULL);

	/* Send IF Condition. */
	v  = bits_set(SDHC_CMD8_PATTERN, 0xaa);
	v |= bits_set(SDHC_CMD8_VHS, SDHC_CMD8_VHS_27_36);
	sdhc_send_command(SDHC_CMD8, v, resp);

	v = resp[0];
	assert(bits_get(v, SDHC_CMD8_PATTERN) == 0xaa);
	assert(bits_get(v, SDHC_CMD8_VHS));

	/* Send OP Condition. */
	sdhc_send_command(SDHC_CMD55, 0, resp);
	/* QRPI2 sends 0x120 as the card status. */
	v = bits_on(SDHC_ACMD41_VDD_32_33);
	sdhc_send_command(SDHC_ACMD41, v, resp);
	/* QRPI2 sends 0x80ffff00 as the OCR. */
	assert(bits_get(resp[0], SDHC_ACMD41_VDD_32_33));

	/* Send All CID. */
	sdhc_send_command(SDHC_CMD2, 0, resp);
	//sd_parse_cid(resp);

	/* Send RCA. */
	sdhc_send_command(SDHC_CMD3, 0, resp);
	addr = bits_get(resp[0], SDHC_CMD3_RCA);
	(void)addr;

	/* Send CSD. */
	v = bits_set(SDHC_CMD9_RCA, addr);
	sdhc_send_command(SDHC_CMD9, v, resp);
	//sd_parse_csd(resp);

	/* Select card. */
	v = bits_set(SDHC_CMD7_RCA, addr);
	sdhc_send_command(SDHC_CMD7, v, resp);
	uart_send_num(resp[0]);
	/* Card Status is 0x700 - standby mode, ready for data. */

	/* Try reading the first block. */
	sdhc_send_command(SDHC_CMD17, 0, resp);
	uart_send_num(resp[0]);
	/* Card Status is 0x900 - transfer mode, ready for data. */

	for (int i = 0; i < 512 / 4; ++i) {
		v = sdhc_read_data();
		if (v)
			uart_send_num(v);
	}
}
