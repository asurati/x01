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
#include <bdy.h>
#include <mmu.h>
#include <pm.h>
#include <string.h>
#include <mutex.h>

#include <sys/mmu.h>

static struct bdy bdy_ram;
static struct page *ram_map;
static struct mutex ram_map_lock;
static size_t ramsz;

static int	_pm_ram_alloc(enum pm_alloc_units unit, int n, uintptr_t *pa);

void pm_init(uint32_t ram, uint32_t _ramsz)
{
	int i, mapsz, npfns, ret;
	size_t _128mb;
	uintptr_t pa[4], va, t;
	struct mmu_map_req r;
	extern char bdy_map_start;
	extern char ram_map_start;
	extern char text_start, text_end;
	extern char rodata_start, rodata_end;
	extern char data_start, data_end;
	extern char bss_start, bss_end;
	extern char ptab_start, ptab_end;
	struct section {
		uintptr_t start;
		uintptr_t end;
	} const si[] = {
		{(uintptr_t)&text_start,	(uintptr_t)&text_end},
		{(uintptr_t)&rodata_start,	(uintptr_t)&rodata_end},
		{(uintptr_t)&data_start,	(uintptr_t)&data_end},
		{(uintptr_t)&bss_start,		(uintptr_t)&bss_end},
		{(uintptr_t)&ptab_start,	(uintptr_t)&ptab_end},
		/* TODO add excpt_start PAGE. */
	};

	mutex_init(&ram_map_lock);
	ramsz = _ramsz;


	/* For RPi, the RAM start is expected to be at physical zero. */
	assert(ram == 0);
	assert(ramsz);

	/* Restrict RAM usage to 128MB. */
	_128mb = 128 * 1024 * 1024;
	if (ramsz > _128mb)
		ramsz = _128mb;

	/* The map must be a maximum of 2 pages long. */
	npfns = ramsz >> PAGE_SIZE_SZ;
	mapsz = ALIGN_UP(bdy_map_size(npfns), PAGE_SIZE);
	mapsz >>= PAGE_SIZE_SZ;
	assert(mapsz > 0 && mapsz < 3);
	bdy_init(&bdy_ram, (void *)&bdy_map_start, npfns);

	/* Reserve the first 8MB of RAM. */
	ret = _pm_ram_alloc(PM_UNIT_2MB, 4, pa);
	assert(ret == 0);
	for (i = 0; i < 4; ++i)
		assert(pa[i] == (size_t)i * 2 * 1024 * 1024);

	ram_map = (struct page *)&ram_map_start;

	mapsz = ALIGN_UP(npfns * sizeof(struct page), PAGE_SIZE);

	/* 128MB RAM is equivalent to 32768 4KB frames. The current
	 * sizeof(struct page) is 8 bytes, which results in a
	 * ram_map of size 256KB. To allow growing the struct page,
	 * the maximum size the map can consume is fixed at 1MB.
	 */
	assert(mapsz <= 1024 * 1024);

	ret = _pm_ram_alloc(PM_UNIT_SECTION, 1, pa);
	assert(ret == 0);

	r.va_start = ram_map;
	r.pa_start = pa[0];
	r.n = 1;
	r.mt = MT_NRM_IO_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_SECTION;
	r.flags  = bits_on(MMR_XN);
	r.flags |= bits_on(MMR_AB);	/* Prevent access faults on ram_map. */

	ret = mmu_map(&r);
	assert(ret == 0);

	memset(ram_map, 0, mapsz);

	/* The pages used to map the sections (one of which includes the
	 * pages for k_pd, k_pt, k_pta_pt and ram_map_pt) must be filled
	 * into the array. The physical addresses for these statically
	 * allocated sections are easy to calculate without parsing the page
	 * tables.
	 *
	 * Moreover, the data pages of the ram_map must be filled in.
	 */

	for (i = 0; (unsigned)i < ARRAY_SIZE(si); ++i) {
		for (va = si[i].start; va < si[i].end; va += PAGE_SIZE) {
			t = va - kmode_va;
			t >>= PAGE_SIZE_SZ;
			ram_map[t].flags |= bits_set(PGF_UNIT, PM_UNIT_PAGE);
			ram_map[t].u0.ref = 1;
		}
	}

	/* There are 256 4KB contigous pages in a 1MB allocation. */
	t = pa[0] >> PAGE_SIZE_SZ;
	for (i = 0; i < 256; ++i) {
		ram_map[t + i].flags |= bits_set(PGF_UNIT, PM_UNIT_SECTION);
		ram_map[t + i].u0.ref = 1;
	}

	/* After this point, the pm_ram_alloc and pm_ram_free routines
	 * can be used.
	 */
}

static int _pm_ram_alloc(enum pm_alloc_units unit, int n, uintptr_t *pa)
{
	int i, ret, pos;
	assert(unit < PM_UNIT_MAX);

	ret = -1;
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

int pm_ram_alloc(enum pm_alloc_units unit, enum pm_page_usage use, int n,
		 uintptr_t *pa)
{
	int i, j, ret;
	uintptr_t t;
	struct page *pg;

	mutex_lock(&ram_map_lock);
	ret = _pm_ram_alloc(unit, n, pa);
	if (ret)
		goto exit;

	for (i = 0; i < n; ++i) {
		t = pa[i] >> PAGE_SIZE_SZ;
		for (j = 0; j < (1 << unit); ++j) {
			pg = &ram_map[t + j];
			memset(pg, 0, sizeof(*pg));
			pg->flags |= bits_set(PGF_UNIT, unit);
			pg->flags |= bits_set(PGF_USE, use);
			if (use == PGF_USE_NORMAL)
				ram_map[t + j].u0.ref = 1;
		}
	}
exit:
	mutex_unlock(&ram_map_lock);
	return ret;
}


int pm_ram_free(enum pm_alloc_units unit, enum pm_page_usage use, int n,
		const uintptr_t *pa)
{
	int i, j, ret;
	uintptr_t p, mask;
	struct page *pg;

	assert(unit < PM_UNIT_MAX);

	mutex_lock(&ram_map_lock);
	mask = (1 << unit) - 1;
	for (i = 0; i < n; ++i) {
		/*
		ret = -1;
		*/
		p = pa[i] >> PAGE_SIZE_SZ;

		/* The input addresses must be aligned according to
		 * the unit passed.
		 */
		assert((p & mask) == 0);

		/* The struct page flags must reflect the unit, the use,
		 * and an appropriate ref count.
		 */
		for (j = 0; j < (1 << unit); ++j) {
			pg = &ram_map[p + j];
			assert(bits_get(pg->flags, PGF_UNIT) == unit);
			assert(bits_get(pg->flags, PGF_USE) == use);
			if (use == PGF_USE_NORMAL)
				assert(ram_map[p + j].u0.ref == 1);
		}

		/* The page must be busy in the buddy bitmap. */
		ret = bdy_free(&bdy_ram, unit, p >> unit);
		assert(ret == 0);

		for (j = 0; j < (1 << unit); ++j)
			ram_map[p + j].u0.ref = 0;
	}

	mutex_unlock(&ram_map_lock);
	ret = 0;
	return ret;
}

/* TODO: Reference counting. */
struct page *pm_ram_get_page(uintptr_t pa)
{
	struct page *pg;

	assert(pa < ramsz);
	pa >>= PAGE_SIZE_SZ;
	mutex_lock(&ram_map_lock);
	pg = &ram_map[pa];
	mutex_unlock(&ram_map_lock);
	return pg;
}
