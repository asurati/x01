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
#include <slub.h>
#include <sched.h>
#include <irq.h>
#include <fb.h>
#include <list.h>
#include <string.h>
#include <uart.h>

void sched_current_init();
void sched_init();
void mmu_init();
void pm_init(uint32_t ram, uint32_t ramsz);
void io_init();
void slub_init();
void vm_init();
void excpt_init();
void intc_init();
void irq_init();
void ioreq_init();
void timer_init();
void timer_start();
void mbox_init();
void fb_init();
void uart_init();
void sdhc_init();

#ifdef QRPI2

static int display_thread(void *p)
{
	void *q;
	const int sz = 64;
	int i, row, colsz;
	uint8_t *line[2];

	(void)p;

	colsz = (fbi->depth >> 3) * sz;
	line[0] = kmalloc(colsz);
	line[1] = kmalloc(colsz);

	assert(line[0] && line[1]);

	memset(line[0], 0, colsz);
	memset(line[1], 0, colsz);


	for (i = 0; i < colsz; i += (fbi->depth >> 3)) {
		line[0][i] = 0xff;		/* Red */
		line[1][i + 2] = 0xff;		/* Blue */
	}

	i = 0;
	while (1) {
		p = fb;
		q = p + fbi->pitch - colsz;
		for (row = 0; row < sz; ++row) {
			memcpy(p, line[i], colsz);
			memcpy(q, line[i], colsz);
			p += fbi->pitch;
			q += fbi->pitch;
		}

		p = fb + (fbi->phy_height - sz) * fbi->pitch;
		q = p + fbi->pitch - colsz;
		for (row = 0; row < sz; ++row) {
			memcpy(p, line[i], colsz);
			memcpy(q, line[i], colsz);
			p += fbi->pitch;
			q += fbi->pitch;
		}

		/* Since the only interrupt active is the timer interrupt, which
		 * fires every second, the loop wakes up every second to change
		 * the colour of the box.
		 */
		msleep(1000);
		i = !i;
	}
	return 0;
}

#endif

static int ticker_thread(void *p)
{
	int ticks = 0;

	(void)p;

	while (1) {
		uart_send_num(ticks);
		msleep(1000);
		++ticks;
	}
	return 0;
}

void kmain()
{
	struct list_head wq;
	uint32_t ram, ramsz;
	extern uint32_t _ram, _ramsz;

	/* Read before the identity map is destroyed. */
	ram = _ram;
	ramsz = _ramsz;
	barrier();

	sched_current_init();
	mmu_init();
	io_init();
	pm_init(ram, ramsz);
	slub_init();
	vm_init();
	excpt_init();
	intc_init();
	irq_init();
	sched_init();
	ioreq_init();
	timer_init();
	mbox_init();

	/* Enable IRQs once hard and soft IRQs are setup. */
	irq_enable();

	timer_start();

	/* The following init() calls require IRQs to be enabled,
	 * since they involve IO to the MBOX.
	 */

#ifdef QRPI2
	fb_init();
	uart_init();
#endif

#ifdef QRPI2
	(void)display_thread;
	sched_thread_create(display_thread, NULL);
#endif
	sched_thread_create(ticker_thread, NULL);

	sdhc_init();

	init_list_head(&wq);

	/* Remove this thread from scheduling. */
	wait_event(&wq, 0);
}
