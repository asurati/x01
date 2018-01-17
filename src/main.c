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

#include <mmu.h>
#include <pm.h>
#include <slub.h>
#include <vm.h>
#include <io.h>
#include <excpt.h>
#include <irq.h>
#include <timer.h>
#include <sched.h>

int thr0(void *p);
void kmain(const void *al)
{
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
	irq_init();
	timer_init();
	sched_init();
	sched_thread_create(thr0, (void *)0xdeaddead);

	while (1)
		asm volatile("wfi");
}

int thr0(void *p)
{
	p = p;
	while (1)
		asm volatile("wfi");
	return 0;
}

