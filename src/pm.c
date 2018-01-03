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

void pm_init(uint32_t ram, uint32_t ramsz)
{
	int i, mapsz, npfns;
	size_t _128mb;
	uintptr_t pa[4];
	extern char bdy_map_start;

	/* For RPi, the RAM start is expected to be at physical zero. */
	assert(ram == 0);
	assert(ramsz);

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

	/* Reserve the first 8MB of RAM. */
	pm_ram_alloc(PM_UNIT_2MB, 4, pa);
	for (i = 0; i < 4; ++i)
		assert(pa[i] == (size_t)i * 2 * 1024 * 1024);
}

int pm_ram_alloc(enum pm_alloc_units unit, int n, uintptr_t *pa)
{
	int i, ret, pos;
	assert(unit < PM_UNIT_MAX);

	for (i = 0; i < n; ++i) {
		ret = bdy_alloc(&bdy_ram, unit, &pos);
		if (ret)
			break;
		pa[i] = pos;
	}

	if (ret == 0) {
		for (i = 0; i < n; ++i) {
			pa[i] <<= unit;
			pa[i] <<= PAGE_SIZE_SZ;
		}
		return ret;
	}

	for (i = i - 1; i >= 0; --i)
		bdy_free(&bdy_ram, unit, pa[i]);
	return ret;
}

int pm_ram_free(enum pm_alloc_units unit, int n, const uintptr_t *pa)
{
	int i, ret;
	uintptr_t p, mask;
	assert(unit < PM_UNIT_MAX);

	/* The input addresses must be aligned according to the unit passed. */
	mask = (1 << unit) - 1;
	for (i = 0; i < n; ++i) {
		ret = -1;
		p = pa[i] >> PAGE_SIZE_SZ;
		if (p & mask)
			break;
		p >>= unit;
		ret = bdy_free(&bdy_ram, unit, p);
		if (ret)
			break;
	}
	return ret;
}
