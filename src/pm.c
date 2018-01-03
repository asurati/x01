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
#include <string.h>

struct bdy bdy_ram;

static uintptr_t ram_end_pa;

void pm_init(uint32_t ram, uint32_t ramsz)
{
	int mapsz, npfns;
	size_t _128mb;
	extern char ram_static_use_end_pa;
	extern char bdy_map_start;

	/* For RPi, the RAM start is expected to be at physical zero. */
	assert(ram == 0);
	assert(ramsz);

	ram_end_pa = (uintptr_t)&ram_static_use_end_pa;

	/* Restrict RAM usage to 128MB. */
	_128mb = 128 * 1024 * 1024;
	if (ramsz > _128mb)
		ramsz = _128mb;

	/* The map must be a maximum of 4 pages long. */
	npfns = ramsz >> PAGE_SIZE_SZ;
	mapsz = ALIGN_UP(bdy_map_size(npfns), PAGE_SIZE);
	mapsz >>= PAGE_SIZE_SZ;
	assert(mapsz > 0 && mapsz < 5);
	bdy_init(&bdy_ram, (void *)&bdy_map_start, npfns);
}
