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
#include <lock.h>

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

static int io_pending;
static struct io_req *req_pending;

static struct lock ioq_lock;
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

_ctx_hard
static int mbox_irq(void *data)
{
	(void)data;

	if (BF_GET(readl(&ro->config), MBOX_RO_IRQ_PEND)) {
		/* Silence the interrupt by reading the RW field. */
		readl(&ro->rw);
		irq_sched_raise(IRQ_SCHED_MBOX);
	}
	return 0;
}

static void mbox_io_process();
_ctx_sched
static int mbox_irq_sched(void *data)
{
	(void)data;
	/* The setting of the DONE bit will be visible to the sleeping
	 * process latest at the preempt_enable() release performed
	 * by wake_up().
	 */
	BF_SET(req_pending->flags, IORF_DONE, 1);
	wake_up(req_pending->wq);

	/* Cannot touch the request once the wake up
	 * has been signalled.
	 */
	req_pending = NULL;
	if (io_pending) {
		--io_pending;
		mbox_io_process(list_del_head(&ioq));
	}
	return 0;
}

_ctx_sched
static void mbox_io_process(struct list_head *e)
{
	int ch, code;
	uintptr_t pa;

	assert(req_pending == NULL);
	req_pending = list_entry(e, struct io_req, entry);

	/* Error error checking. */

	code = BF_GET(req_pending->u.ioctl.code, IOCTL_CODE);
	switch (code) {
	case MBOX_IOCTL_FB_ALLOC:
		ch = MBOX_CH_FB;
		break;
	default:
		ch = MBOX_CH_PROP;
		break;
	}

	mmu_dcache_clean_inv(req_pending->req, req_pending->sz);

	pa = (uintptr_t)req_pending->drv_data;
	pa |= ch;

	barrier();

	while (readl(&wo->status) & 0x80000000)
		;

	/* Trigger the IO. */
	writel(pa, &wo->rw);
}

/* Process context only. */
int mbox_io(struct io_req *r)
{
	uintptr_t pa;

	BF_SET(r->flags, IORF_DONE, 0);
	r->ret = 0;

	assert(BF_GET(r->flags, IORF_TYPE) == IORF_TYPE_IOCTL);

	/* va_to_pa must be called in the process context. */
	pa = mmu_va_to_pa(r->req);
	assert(ALIGNED(pa, 16));
	r->drv_data = (void *)pa;

	lock_sched_lock(&ioq_lock);
	if (io_pending == 0) {
		mbox_io_process(&r->entry);
	} else {
		list_add_tail(&r->entry, &ioq);
		++io_pending;
	}
	lock_sched_unlock(&ioq_lock);

	return IO_RET_PENDING;
}

void mbox_init()
{
	uint32_t v;

	init_list_head(&ioq);
	io_pending = 0;

	wo = io_base + MBOX_ARM_WO;
	ro = io_base + MBOX_ARM_RO;

	assert(sizeof(struct mbox) == 0x20);

	irq_hard_insert(IRQ_HARD_MBOX, mbox_irq, NULL);
	irq_sched_insert(IRQ_SCHED_MBOX, mbox_irq_sched, NULL);

	/* QRPI2 supports raising interrupts when VC responds to
	 * ARM's requests.
	 */
	v = 0;
	BF_SET(v, MBOX_RO_IRQ_EN, 1);
	writel(v, &ro->config);

	return;
}
