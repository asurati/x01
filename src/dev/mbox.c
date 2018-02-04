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
#include <mbox.h>
#include <mmu.h>
#include <slub.h>
#include <string.h>
#include <ioreq.h>

#include <sys/ioreq.h>
#include <sys/mbox.h>

static struct mbox *wo;
static struct mbox *ro;
static struct io_req_queue mbox_ioq;

_ctx_hard
static int mbox_irq(void *p)
{
	int ret;
	(void)p;

	ret = IRQH_RET_NONE;
	if (bits_get(readl(&ro->config), MBOX_RO_IRQ_PEND)) {
		/* Silence the interrupt by reading the RW field. */
		readl(&ro->rw);
		irq_sched_raise(IRQ_SCHED_MBOX);
		ret = IRQH_RET_HANDLED | IRQH_RET_SCHED;
	}
	return ret;
}

_ctx_sched
static int mbox_irq_sched(void *p)
{
	struct io_req *ior;

	ior = ioq_ior_dequeue_sched(p);
	ioq_ior_done_sched(p, ior);
	return 0;
}

/* Although the IO routines can run at _ctx_proc, the IO chaining requires
 * them to run at _ctx_sched level too. Consider _ctx_sched as the level
 * at which they run.
 */
_ctx_sched
static void mbox_ioctl(struct io_req *ior)
{
	uintptr_t pa;
	int ch;
	size_t sz;

	sz = 0;
	ch = 0;

	switch (ior->io.ioctl.cmd) {
	case MBOX_IOCTL_FB_ALLOC:
		sz = sizeof(struct mbox_fb_buf);
		ch = MBOX_CH_FB;
		break;
	case MBOX_IOCTL_UART_CLOCK:
		sz = sizeof(struct mbox_prop_buf);
		ch = MBOX_CH_PROP;
		break;
	default:
		assert(0);
		break;
	}

	pa = mmu_va_to_pa(ior->io.ioctl.arg);
	assert(ALIGNED(pa, 16));
	pa |= ch;

	mmu_dcache_clean_inv(ior->io.ioctl.arg, sz);

	barrier();

	while (readl(&wo->status) & 0x80000000)
		;

	/* Trigger the IO. */
	writel(pa, &wo->rw);
}

_ctx_proc
uint32_t mbox_clk_rate_get(enum mbox_clock c)
{
	uint32_t rate;
	struct io_req ior;
	struct mbox_prop_buf *b;

	assert(c > 0 && c < MBOX_CLK_MAX);

	b = kmalloc(sizeof(*b));
	memset(b, 0, sizeof(*b));
	b->sz = sizeof(*b);
	b->u.clk_rate.hdr.id = 0x30002;
	b->u.clk_rate.hdr.sz = 8;
	b->u.clk_rate.id = c;

	memset(&ior, 0, sizeof(ior));
	ior.type = IOR_TYPE_IOCTL;
	ior.io.ioctl.cmd = MBOX_IOCTL_UART_CLOCK;
	ior.io.ioctl.arg = b;

	ioq_ior_submit(&mbox_ioq, &ior);
	ioq_ior_wait(&ior);

	rate = b->u.clk_rate.rate;
	kfree(b);
	return rate;
}

/* The caller must ensure 16-byte alignment. */
_ctx_proc
int mbox_fb_alloc(const struct mbox_fb_buf *b)
{
	struct io_req ior;

	memset(&ior, 0, sizeof(ior));

	ior.type = IOR_TYPE_IOCTL;
	ior.io.ioctl.cmd = MBOX_IOCTL_FB_ALLOC;
	ior.io.ioctl.arg = (void *)b;

	ioq_ior_submit(&mbox_ioq, &ior);
	ioq_ior_wait(&ior);
	return 0;
}

_ctx_init
void mbox_init()
{
	uint32_t v;

	ioq_init(&mbox_ioq, mbox_ioctl, NULL);

	wo = io_base + MBOX_ARM_WO;
	ro = io_base + MBOX_ARM_RO;

	assert(sizeof(struct mbox) == 0x20);

	irq_hard_insert(IRQ_HARD_MBOX, mbox_irq, NULL);
	irq_sched_insert(IRQ_SCHED_MBOX, mbox_irq_sched, &mbox_ioq);

	/* QRPI2 supports raising interrupts when VC responds to
	 * ARM's requests.
	 */
	v = bits_on(MBOX_RO_IRQ_EN);
	writel(v, &ro->config);

	return;
}
