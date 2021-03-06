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

#ifndef _SYS_SDHC_H_
#define _SYS_SDHC_H_

#include <sys/ioreq.h>

#define SDHC_BASE				0x300000

#define SDHC_ARG2				(SDHC_BASE)
#define SDHC_BLKSZCNT				(SDHC_BASE + 0x04)
#define SDHC_ARG1				(SDHC_BASE + 0x08)
#define SDHC_CMDTM				(SDHC_BASE + 0x0c)
#define SDHC_RESP0				(SDHC_BASE + 0x10)
#define SDHC_RESP1				(SDHC_BASE + 0x14)
#define SDHC_RESP2				(SDHC_BASE + 0x18)
#define SDHC_RESP3				(SDHC_BASE + 0x1c)
#define SDHC_DATA				(SDHC_BASE + 0x20)
#define SDHC_STATUS				(SDHC_BASE + 0x24)
#define SDHC_CNTRL0				(SDHC_BASE + 0x28)
#define SDHC_CNTRL1				(SDHC_BASE + 0x2c)
#define SDHC_INT				(SDHC_BASE + 0x30)
#define SDHC_INT_MASK				(SDHC_BASE + 0x34)
#define SDHC_INT_EN				(SDHC_BASE + 0x38)
#define SDHC_CNTRL2				(SDHC_BASE + 0x3c)
#define SDHC_SLOT_ISR_VER			(SDHC_BASE + 0xfc)

#define SDHC_BLKSZ_POS				 0
#define SDHC_BLKCNT_POS				16

#define SDHC_BLKSZ_SZ				10
#define SDHC_BLKCNT_SZ				16

#define SDHC_TM_BLKCNT_EN_POS			 1
#define SDHC_TM_AUTO_CMD_EN_POS			 2
#define SDHC_TM_DAT_DIR_POS			 4
#define SDHC_TM_MULTI_BLK_POS			 5
#define SDHC_CMD_RESP_TYPE_POS			16
#define SDHC_CMD_CRCCHK_EN_POS			19
#define SDHC_CMD_IXCHK_EN_POS			20
#define SDHC_CMD_ISDATA_POS			21
#define SDHC_CMD_TYPE_POS			22
#define SDHC_CMD_INDEX_POS			24

#define SDHC_CMD_RESP_136			 1
#define SDHC_CMD_RESP_48			 2

#define SDHC_TM_BLKCNT_EN_SZ			 1
#define SDHC_TM_AUTO_CMD_EN_SZ			 2
#define SDHC_TM_DAT_DIR_SZ			 1
#define SDHC_TM_MULTI_BLK_SZ			 1
#define SDHC_CMD_RESP_TYPE_SZ			 2
#define SDHC_CMD_CRCCHK_EN_SZ			 1
#define SDHC_CMD_IXCHK_EN_SZ			 1
#define SDHC_CMD_ISDATA_SZ			 1
#define SDHC_CMD_TYPE_SZ			 2
#define SDHC_CMD_INDEX_SZ			 6

#define SDHC_STS_CMD_INHB_POS			 0
#define SDHC_STS_DAT_INHB_POS			 1
#define SDHC_STS_DAT_ACT_POS			 2
#define SDHC_STS_WRDY_POS			 8
#define SDHC_STS_RRDY_POS			 9
#define SDHC_STS_CARD_IN_POS			16
#define SDHC_STS_CARD_STABLE_POS		17
#define SDHC_STS_SDCD_INV_POS			18
#define SDHC_STS_SDWP_POS			19
#define SDHC_STS_DAT_LEVEL_POS			20
#define SDHC_STS_CMD_LEVEL_POS			24
#define SDHC_STS_DAT_LEVEL1_POS			25

#define SDHC_STS_CMD_INHB_SZ			 1
#define SDHC_STS_DAT_INHB_SZ			 1
#define SDHC_STS_DAT_ACT_SZ			 1
#define SDHC_STS_WRDY_SZ			 1
#define SDHC_STS_RRDY_SZ			 1
#define SDHC_STS_DAT_LEVEL_SZ			 4
#define SDHC_STS_CMD_LEVEL_SZ			 1
#define SDHC_STS_DAT_LEVEL1_SZ			 4

#define SDHC_C0_4BIT_POS			 1
#define SDHC_C0_HS_EN_POS			 2
#define SDHC_C0_8BIT_POS			 5
#define SDHC_C0_SDBUS_PWR_POS			 8
#define SDHC_C0_SDBUS_VOLT_POS			 9

#define SDHC_C0_4BIT_SZ				 1
#define SDHC_C0_HS_EN_SZ			 1
#define SDHC_C0_8BIT_SZ				 1
#define SDHC_C0_SDBUS_PWR_SZ			 1
#define SDHC_C0_SDBUS_VOLT_SZ			 3

#define SDHC_C1_CLK_EN_POS			 0
#define SDHC_C1_CLK_STABLE_POS			 1
#define SDHC_C1_SDCLK_EN_POS			 2
#define SDHC_C1_CLK_GENSEL_POS			 5
#define SDHC_C1_CLKF_HI_POS			 6
#define SDHC_C1_CLKF_LO_POS			 8
#define SDHC_C1_DATA_TOUNIT_POS			16
#define SDHC_C1_RESET_ALL_POS			24
#define SDHC_C1_RESET_CMD_POS			25
#define SDHC_C1_RESET_DATA_POS			26

#define SDHC_C1_CLK_EN_SZ			 1
#define SDHC_C1_CLK_STABLE_SZ			 1
#define SDHC_C1_SDCLK_EN_SZ			 1
#define SDHC_C1_CLK_GENSEL_SZ			 1
#define SDHC_C1_CLKF_HI_SZ			 2
#define SDHC_C1_CLKF_LO_SZ			 8
#define SDHC_C1_DATA_TOUNIT_SZ			 4
#define SDHC_C1_RESET_ALL_SZ			 1
#define SDHC_C1_RESET_CMD_SZ			 1
#define SDHC_C1_RESET_DATA_SZ			 1

#define SDHC_INT_ERR_POS			15
#define SDHC_INT_CMD_POS			 0
#define SDHC_INT_ERR_SZ				 1
#define SDHC_INT_CMD_SZ				 1

#define SDHC_VER_HC_POS				16
#define SDHC_VER_HC_SZ				 8

#define SDHC_IOCTL_COMMAND			 1

#define SDHC_MIN_FREQ			    400000
#define SDHC_SDR12_FREQ			  25000000
#define SDHC_BLK_SIZE			       512

enum sdhc_cmd {
	SDHC_CMD0,
	SDHC_CMD1,
	SDHC_CMD2,
	SDHC_CMD3,

	SDHC_ACMD6 = 6,
	SDHC_CMD7 = 7,
	SDHC_CMD8,
	SDHC_CMD9,

	SDHC_CMD17 = 17,
	SDHC_ACMD41 = 41,
	SDHC_ACMD42,
	SDHC_ACMD51 = 51,
	SDHC_CMD55 = 55
};

#define SDHC_CMD3_RCA_POS			16
#define SDHC_CMD3_RCA_SZ			16

#define SDHC_ACMD6_BUSW_4			 2

#define SDHC_CMD7_RCA_POS			SDHC_CMD3_RCA_POS
#define SDHC_CMD7_RCA_SZ			SDHC_CMD3_RCA_SZ

#define SDHC_CMD8_PATTERN_POS			 0
#define SDHC_CMD8_VHS_POS			 8
#define SDHC_CMD8_PATTERN_SZ			 8
#define SDHC_CMD8_VHS_SZ			 4

#define SDHC_CMD8_VHS_27_36			 1

#define SDHC_CMD9_RCA_POS			SDHC_CMD3_RCA_POS
#define SDHC_CMD9_RCA_SZ			SDHC_CMD3_RCA_SZ

#define SDHC_CMD55_RCA_POS			SDHC_CMD3_RCA_POS
#define SDHC_CMD55_RCA_SZ			SDHC_CMD3_RCA_SZ

#define SDHC_OCR_VDD_32_33_POS			20
#define SDHC_OCR_CS_POS				30
#define SDHC_OCR_BUSY_POS			31
#define SDHC_OCR_VDD_32_33_SZ			 1
#define SDHC_OCR_CS_SZ				 1
#define SDHC_OCR_BUSY_SZ			 1

struct sdhc_softc {
	struct io_req_queue sc_ioq;
	uintptr_t sc_int;
	uint16_t sc_addr;
	uint32_t sc_emmc_clk;
};

struct sdhc_cmd_info {
	enum sdhc_cmd cmd;
	uint32_t arg;
	void *resp;
};

struct sdhc_cid {
        uint8_t mid;
        uint16_t oid;
        uint8_t pnm[5];
        uint8_t prv;
        uint32_t psn;
        uint16_t mdt;
};

struct sdhc_csd {
        uint8_t tran_speed;
        uint8_t dsr;
        uint16_t c_sz;
        uint8_t c_sz_mul;
        uint8_t read_bl_len;
};
#endif
