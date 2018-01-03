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
#include <pm.h>
#include <mmu.h>
#include <string.h>

extern char KMODE_VA;
extern char pt_start;
extern char k_pd_start;

const uintptr_t kmode_va = (uintptr_t)&KMODE_VA;
const uintptr_t pt_area_va = (uintptr_t)&pt_start;

/* The values correspond to mmu_map_unit. */
static const enum pm_alloc_units map_units[MAP_UNIT_MAX] = {
	PM_UNIT_PAGE,
	PM_UNIT_LARGE_PAGE,
	PM_UNIT_SECTION,
	PM_UNIT_SUPER_SECTION
};

/* VA to PTE VA. */
static void *get_pte_va(void *va)
{
	uintptr_t r;
	r  = pt_area_va;
	r += ((uintptr_t)va >> PAGE_SIZE_SZ) << 2;
	return (void *)r;
}

void mmu_dcache_clean_nomcrr(void *va, size_t sz)
{
	uintptr_t i, s, e;

	if (sz <= 0)
		return;

	s = (uintptr_t)va;
	e = s + sz;
	for (i = s; i < e; i += CACHE_LINE_SIZE)
		asm volatile("mcr	p15, 0, %0, c7, c10, 1\n\t"
			     : : "r" (i));
	mmu_dsb();
}

void mmu_dcache_clean_mcrr(void *va, size_t sz)
{
	uintptr_t s, e;

	if (sz <= 0)
		return;

	s = (uintptr_t)va;
	e = s + sz - 1;
	asm volatile("mcrr	p15, 0, %0, %1, c12\n\t"
		     : : "r" (e), "r" (s));
	mmu_dsb();
}

void mmu_tlb_invalidate(void *va, size_t sz)
{
	uintptr_t i, s, e;
	if (sz <= 0)
		return;

	s = ALIGN_DN((uintptr_t)va, PAGE_SIZE);
	e = s + sz;
	for (i = s; i < e; i += PAGE_SIZE) {
		asm volatile("mcr	p15, 0, %0, c8, c5, 1\n\t"
			     : : "r" (i));
		asm volatile("mcr	p15, 0, %0, c8, c6, 1\n\t"
			     : : "r" (i));
	}
	mmu_dsb();
}

void mmu_init()
{
	uint32_t v;
	uintptr_t *pd, *pt, pa, te;
	void *va;
	extern char k_pt_pa;

	/* Disable the table walk for TTBR0. */
	asm volatile("mrc	p15, 0, %0, c2, c0, 2\n\t"
		     : "=r" (v));
	BF_SET(v, TTBCR_PD0, 1);
	asm volatile("mcr	p15, 0, %0, c2, c0, 2\n\t"
		     : : "r" (v));

	/* Remove the 4MB identity map. */
	pd = (uintptr_t *)&k_pd_start;
	memset(pd, 0, 4 * sizeof(pd[0]));

	mmu_dcache_clean(pd, 4 * sizeof(uintptr_t));
	mmu_tlb_invalidate(NULL, 1024 * PAGE_SIZE);

	/* Map k_pt at its appropriate VA witin the PT area. */

	/* va is the calculated PTE VA of the PT mapping
	 * kmode_va.
	 */
	va = get_pte_va((void *)kmode_va);

	/* pt is the calculated PTE VA of the PT mapping
	 * k_pt.
	 */
	pt = get_pte_va(va);
	pa = (uintptr_t)&k_pt_pa;

	te = 0;
        BF_SET(te, PTE_TYPE, 2);        /* Small Page. */
        BF_SET(te, PTE_AP, 1);          /* Supervisor-only. */
        BF_SET(te, PTE_SP_XN, 1);       /* No Execute. */
        BF_SET(te, PTE_C, 1);           /* I/O WB, AoW. */
        BF_SET(te, PTE_B, 1);
        BF_SET(te, PTE_SP_TEX, 1);
        BF_PUSH(te, PTE_SP_BASE, pa);
        pt[0] = te;
	mmu_dcache_clean(pt, sizeof(uintptr_t));
}

int mmu_map(void *_pd, const struct mmu_map_req *r)
{
	int i, j, k, n, ret;
	const int nunits[4] = {1, 16, 1, 16};
	uintptr_t mask, va, pa, inc, *pd, tva, tpa, *pt;
	uintptr_t de, te;
	struct mmu_map_req tr;

	assert(r && r->n > 0);
	assert(r->mt < MT_MAX);
	assert(r->ap < AP_MAX);
	assert(r->mu < MAP_UNIT_MAX);

	if (_pd == NULL)
		pd = (uintptr_t *)&k_pd_start;
	else
		pd = _pd;

	/* The addresses must be aligned corresponding to the unit
	 * requested.
	 */

	mask = (1 << map_units[r->mu]);
	mask <<= PAGE_SIZE_SZ;
	--mask;

	va = (uintptr_t)r->va_start;
	pa = r->pa_start;
	assert((va & mask) == 0);
	assert((pa & mask) == 0);

	inc = 1 << (map_units[r->mu] + PAGE_SIZE_SZ);
	n = nunits[r->mu];

	if (r->mu == MAP_UNIT_SECTION || r->mu == MAP_UNIT_SUPER_SECTION) {
		for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
			/* Prevent overflow. */
			assert(va >= (uintptr_t)r->va_start);
			assert(pa >= r->pa_start);

			mmu_tlb_invalidate((void *)va, inc);

			j = BF_GET(va, VA_PDE_IX);

			/* Ensure that the de are invalid. */
			for (k = 0; k < n; ++k)
				assert(BF_GET(pd[j + k], PDE_TYPE0) == 0);

			de = 0;
			BF_SET(de, PDE_TYPE0, 2);		/* S or SS. */
			if (r->mu == MAP_UNIT_SUPER_SECTION) {
				BF_SET(de, PDE_TYPE1, 1);	/* SS. */
				BF_PUSH(de, PDE_SS_BASE, pa);
			} else {
				BF_SET(de, PDE_DOM, r->domain & 0xf);
				BF_PUSH(de, PDE_S_BASE, pa);
			}

			if (!r->exec)
				BF_SET(de, PDE_XN, 1);
			if (r->shared)
				BF_SET(de, PDE_SHR, 1);
			if (!r->global)
				BF_SET(de, PDE_NG, 1);

			BF_SET(de, PDE_TEX, (r->mt >> 2) & 7);
			BF_SET(de, PDE_C, (r->mt >> 1) & 1);
			BF_SET(de, PDE_B, r->mt & 1);
			BF_SET(de, PDE_APX, (r->ap >> 2) & 1);
			BF_SET(de, PDE_AP, r->ap & 3);

			for (k = 0; k < n; ++k)
				pd[j + k] = de;

			mmu_dcache_clean(&pd[j], n * sizeof(uintptr_t));
		}
	} else {
		for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
			/* Prevent overflow. */
			assert(va >= (uintptr_t)r->va_start);
			assert(pa >= r->pa_start);

			mmu_tlb_invalidate((void *)va, inc);

			/* We link a PAGE_SIZE page as a PT at the 4MB
			 * boundary. Check if the de has a valid PT
			 * populated.
			 */
			tva = ALIGN_DN(va, 4 * 1024 * 1024);
			j = BF_GET(tva, VA_PDE_IX);

			k = BF_GET(pd[j], PDE_TYPE0);
			assert(k == 0 || k == 1);

			/* All 4 de must be either empty or filled with PT. */
			if (k == 0) {
				assert(BF_GET(pd[j + 0], PDE_TYPE0) == 0);
				assert(BF_GET(pd[j + 1], PDE_TYPE0) == 0);
				assert(BF_GET(pd[j + 2], PDE_TYPE0) == 0);
				assert(BF_GET(pd[j + 3], PDE_TYPE0) == 0);
			} else {
				tpa = BF_PULL(pd[j + 0], PDE_PT_BASE);

				/* The 0th de's PT must be PAGE aligned. */
				assert(ALIGNED(tpa, PAGE_SIZE));
				assert(BF_PULL(pd[j + 1], PDE_PT_BASE) ==
				       tpa + 0x400);
				assert(BF_PULL(pd[j + 2], PDE_PT_BASE) ==
				       tpa + 0x400 * 2);
				assert(BF_PULL(pd[j + 3], PDE_PT_BASE) ==
				       tpa + 0x400 * 3);
			}

			/* If the de are empty, assign and map a new PT. */
			if (k == 0) {
				memset(&tr, 0, sizeof(tr));
				tr.n = 1;
				tr.va_start = get_pte_va((void *)tva);
				assert(ALIGNED((uintptr_t)tr.va_start,
					       PAGE_SIZE));

				ret = pm_ram_alloc(PM_UNIT_PAGE, 1,
						   &tr.pa_start);
				assert(ret == 0);

				tr.global = 1;
				tr.mt = MT_NRM_WBA;
				tr.ap = AP_SRW;
				tr.mu = MAP_UNIT_PAGE;
				ret = mmu_map(NULL, &tr);
				assert(ret == 0);
				memset(tr.va_start, 0, PAGE_SIZE);

				for (k = 0; k < 4; ++k) {
					de = 0;
					BF_SET(de, PDE_TYPE0, 1);
					BF_PUSH(de, PDE_PT_BASE, tr.pa_start);
					pd[j + k] = de;
					tr.pa_start += 0x400;
				}
			}

			pt = get_pte_va((void *)va);

			/* Ensure that the te are invalid. */
			for (k = 0; k < n; ++k)
				assert(BF_GET(pt[k], PTE_TYPE) == 0);

			te = 0;
			if (r->mu == MAP_UNIT_LARGE_PAGE) {
				BF_SET(te, PTE_TYPE, 1);
				BF_PUSH(de, PTE_LP_BASE, pa);
				if (!r->exec)
					BF_SET(te, PTE_LP_XN, 1);
				BF_SET(te, PTE_LP_TEX, (r->mt >> 2) & 7);
			} else {
				BF_SET(te, PTE_TYPE, 2);
				BF_PUSH(te, PTE_SP_BASE, pa);
				if (!r->exec)
					BF_SET(te, PTE_SP_XN, 1);
				BF_SET(te, PTE_SP_TEX, (r->mt >> 2) & 7);
			}

			if (r->shared)
				BF_SET(te, PTE_SHR, 1);
			if (!r->global)
				BF_SET(te, PTE_NG, 1);

			BF_SET(te, PTE_C, (r->mt >> 1) & 1);
			BF_SET(te, PTE_B, r->mt & 1);
			BF_SET(te, PTE_APX, (r->ap >> 2) & 1);
			BF_SET(te, PTE_AP, r->ap & 3);

			for (k = 0; k < n; ++k)
				pt[k] = te;

			mmu_dcache_clean(&pt[j], n * sizeof(uintptr_t));
		}
	}
	return 0;
}
