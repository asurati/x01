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
#include <mmu.h>
#include <pm.h>
#include <slub.h>
#include <vm.h>
#include <io.h>
#include <intc.h>
#include <excpt.h>
#include <irq.h>
#include <timer.h>
#include <sched.h>
#include <mbox.h>
#include <fb.h>
#include <list.h>
#include <string.h>
#include <uart.h>

static int display_thread(void *p)
{
	void *q;
	struct fb_info fbi;
	const int sz = 64;
	int i, row, colsz;
	uint8_t *line[2];

	p = p;

	fb_info_get(&fbi);

	colsz = (fbi.depth >> 3) * sz;
	line[0] = kmalloc(colsz);
	line[1] = kmalloc(colsz);

	memset(line[0], 0, colsz);
	memset(line[1], 0, colsz);

	for (i = 0; i < colsz; i += (fbi.depth >> 3)) {
		line[0][i] = 0xff;		/* Red */
		line[1][i + 2] = 0xff;		/* Blue */
	}

	i = 0;
	while (1) {
		p = fbi.addr;
		q = p + fbi.pitch - colsz;
		for (row = 0; row < sz; ++row) {
			memcpy(p, line[i], colsz);
			memcpy(q, line[i], colsz);
			p += fbi.pitch;
			q += fbi.pitch;
		}

		p = fbi.addr + (fbi.height - sz) * fbi.pitch;
		q = p + fbi.pitch - colsz;
		for (row = 0; row < sz; ++row) {
			memcpy(p, line[i], colsz);
			memcpy(q, line[i], colsz);
			p += fbi.pitch;
			q += fbi.pitch;
		}

		/* Since the only interrupt active is the timer interrupt, which
		 * fires every second, the loop wakes up every second to change
		 * the colour of the box.
		 */
		asm volatile("wfi");
		i = !i;
	}
	return 0;
}

void kmain(const void *al)
{
	struct list_head wq;
	uint32_t ram, ramsz;
	const uint32_t *p = al;

	/* Search the atag list for RAM details. */
	ram = ramsz = 0;
	while (1) {
		if (p[0] == 0 && p[1] == 0)
			break;

		/* ATAG_MEM */
		if (p[1] == 0x54410002) {
			ramsz = p[2];
			ram = p[3];
			break;
		}
		p += p[0];
	}

	mmu_init();
	pm_init(ram, ramsz);
	slub_init();
	vm_init();
	io_init();
	excpt_init();
	intc_init();
	irq_init();
	sched_init();
	timer_init();
	mbox_init();

	/* Enable IRQs once hard and soft IRQs are setup. */
	irq_enable();

	/* The following init() calls require IRQs to be enabled,
	 * since they involve IO to the MBOX.
	 */

	fb_init();
	uart_init();

	sched_thread_create(display_thread, NULL);

	init_list_head(&wq);

	/* Remove this thread from scheduling. */
	wait_event(&wq, 0);
}
