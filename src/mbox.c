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
#include <list.h>
#include <mbox.h>
#include <mmu.h>
#include <sched.h>
#include <slub.h>
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

#define MBOX_CH_FB		1
#define MBOX_CH_PROP		8

#define MBOX_TAG_MAC		0x10003
#define MBOX_TAG_END		0

#define MBOX_TAG_TYPE_POS	31
#define MBOX_TAG_TYPE_SZ	1

static struct list_head ioq;

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

struct mbox_prop_buf {
	uint32_t sz;
	uint32_t code;
	union {
		struct mbox_tag_mac mac;
	} u;
	uint32_t end;
};

struct mbox_fb_buf {
	uint32_t pw;
	uint32_t ph;
	uint32_t vw;
	uint32_t vh;
	uint32_t pitch;
	uint32_t depth;
	uint32_t vx;
	uint32_t vy;
	uint32_t addr;
	uint32_t sz;
};

static int mbox_irq(void *data)
{
	data = data;

	if (BF_GET(readl(&ro->config), MBOX_RO_IRQ_PEND)) {
		/* Silence the interrupt by reading the RW field. */
		readl(&ro->rw);
		irq_soft_raise(IRQ_SOFT_MBOX);
	}
	return 0;
}

/* Temporary hack before the IO manager is implemented. */
static int signal;
static int mbox_irq_soft(void *data)
{
	data = data;
	signal = 1;
	wake_up(&ioq);
	return 0;
}

int mbox_fb_alloc(int width, int height, int depth, uintptr_t *addr,
		  size_t *sz, int *pitch)
{
	uintptr_t pa;
	struct mbox_fb_buf *b;

	/* All kmalloc allocations > 8 are 16-byte aligned. */
	b = kmalloc(sizeof(*b));
	memset(b, 0, sizeof(*b));

	b->pw = b->vw = width;
	b->ph = b->vh = height;
	b->depth = depth;

	mmu_dcache_clean_inv(b, sizeof(*b));

	pa = mmu_va_to_pa(NULL, b);
	assert(ALIGNED(pa, 16));
	pa |= MBOX_CH_FB;

	while (readl(&wo->status) & 0x80000000)
		;

	writel(pa, &wo->rw);
	wait_event(&ioq, signal == 1);

	/* wait_event implies barriers. */
	signal = 0;

	/* TODO Check return parameters. */
	*addr = b->addr;
	*sz = b->sz;
	*pitch = b->pitch;

	kfree(b);
	return 0;
}

void mbox_init()
{
	uint32_t v;

	init_list_head(&ioq);

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

	return;
}
