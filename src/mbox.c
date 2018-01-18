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
#include <mmu.h>
#include <string.h>

#define MBOX_BASE		0xb880
#define MBOX_ARM_RO		(MBOX_BASE)
#define MBOX_ARM_WO		(MBOX_BASE + 0x20)

#define MBOX_RO_IRQ_EN_POS	0
#define MBOX_RO_IRQ_PEND_POS	4
#define MBOX_RO_IRQ_EN_SZ	1
#define MBOX_RO_IRQ_PEND_SZ	1

#define MBOX_BUF_TYPE_POS	31
#define MBOX_BUF_TYPE_SZ	1

#define MBOX_TAG_END		0

#define MBOX_TAG_TYPE_POS	31
#define MBOX_TAG_TYPE_SZ	1

struct mbox {
	uint32_t rw;
	uint32_t r[3];
	uint32_t peek;
	uint32_t sender;
	uint32_t status;
	uint32_t config;
};

static struct mbox *wo;
static struct mbox *ro;

struct mbox_tag {
	uint32_t id;
	uint32_t sz;
	uint32_t type;
};

struct mbox_tag_mac {
	struct mbox_tag hdr;
	uint8_t addr[6];
	uint8_t pad[2];
};

struct mbox_buf {
	uint32_t sz;
	uint32_t code;
	union {
		struct mbox_tag_mac mac;
	} u;
	uint32_t end;
};

static int mbox_irq(void *data)
{
	data = data;

	if (BF_GET(readl(&ro->config), MBOX_RO_IRQ_PEND))
		readl(&ro->rw);
	return 0;
}

static int mbox_irq_soft(void *data)
{
	data = data;
	return 0;
}

struct mbox_buf buf __attribute__ ((aligned (16)));

void mbox_init()
{
	uint32_t v;
	uintptr_t pa;

	wo = io_base + MBOX_ARM_WO;
	ro = io_base + MBOX_ARM_RO;

	assert(sizeof(struct mbox) == 0x20);

	irq_insert(IRQ_HARD_MBOX, mbox_irq, NULL);
	irq_soft_insert(IRQ_SOFT_MBOX, mbox_irq_soft, NULL);

	/* QRPI2 supports raising interrupts when VC responds to
	 * ARM's requests.
	 */
	v = 0;
	BF_SET(v, MBOX_RO_IRQ_EN, 1);
	writel(v, &ro->config);

	buf.sz = sizeof(buf);
	buf.u.mac.hdr.id = MBOX_TAG_MAC;
	buf.u.mac.hdr.sz = 6;

	mmu_dcache_clean(&buf, sizeof(buf));

	pa = mmu_va_to_pa(NULL, &buf);
	assert(ALIGNED(pa, 16));
	pa |= 8;

	while (1) {
		v = readl(&wo->status);
		if (v & 0x80000000)
			continue;
		else
			break;
	}

	writel(pa, &wo->rw);

	return;
	while (1) {
		v = readl(&ro->status);
		if (v & 0x40000000)
			continue;
		else
			break;
	}

	v = ro->rw;
	assert((v & 0xf) == 8);
	mmu_dsb();

	while (1)
		asm volatile("wfi");
}
