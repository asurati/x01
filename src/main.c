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

	ram = ram;
	if (ramsz == 0)
		goto hang;

	mmu_init();
hang:
	while (1)
		asm volatile("wfi");
}

