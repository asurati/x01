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

#ifndef _SDHC_H_
#define _SDHC_H_

enum sdhc_cmd {
	SDHC_CMD0 = 0,
	SDHC_CMD2 = 2,
	SDHC_CMD3 = 3,
	SDHC_CMD8 = 8,
	SDHC_CMD9 = 9,
	SDHC_ACMD41 = 41,
	SDHC_CMD55 = 55
};

#define SDHC_CMD3_RCA_POS			16
#define SDHC_CMD3_RCA_SZ			16

#define SDHC_CMD8_PATTERN_POS			 0
#define SDHC_CMD8_VHS_POS			 8
#define SDHC_CMD8_PATTERN_SZ			 8
#define SDHC_CMD8_VHS_SZ			 4

#define SDHC_CMD8_VHS_27_36			 1

#define SDHC_CMD9_RCA_POS			16
#define SDHC_CMD9_RCA_SZ			16

#define SDHC_ACMD41_VDD_32_33_POS		20
#define SDHC_ACMD41_VDD_32_33_SZ		 1

int	sdhc_send_command(enum sdhc_cmd, uint32_t arg, void *resp);
#endif
