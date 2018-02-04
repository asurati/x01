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
#include <lock.h>
#include <barrier.h>
#include <slub.h>
#include <uart.h>

#include <sys/mmu.h>
#include <sys/slub.h>

extern char KMODE_VA;
extern char k_pd_start;
extern char k_pt_start;

/* Switch from a mutex to scheduler lock, to allow IO routines
 * to call into mmu at _ctx_proc and _ctx_sched levels.
 */
static struct lock k_pd_lock;

const uintptr_t kmode_va = (uintptr_t)&KMODE_VA;

/* The values correspond to mmu_map_unit. */
static const enum pm_alloc_units map_units[MAP_UNIT_MAX] = {
	PM_UNIT_PAGE,
	PM_UNIT_LARGE_PAGE,
	PM_UNIT_SECTION,
	PM_UNIT_SUPER_SECTION
};

const int pm_alloc_nunits[4] = {1, 16, 1, 16};

void mmu_tlb_invalidate(void *va, size_t sz)
{
	uintptr_t i, s, e;
	if (sz == 0)
		return;

	s = ALIGN_DN((uintptr_t)va, PAGE_SIZE);
	e = s + sz;
	for (i = s; i < e; i += PAGE_SIZE) {
		asm volatile("mcr	p15, 0, %0, c8, c5, 1\n\t"
			     : : "r" (i));
		asm volatile("mcr	p15, 0, %0, c8, c6, 1\n\t"
			     : : "r" (i));
	}

	dsb();

	/* B2.7.3 says a PrefetchFlush is required if the data TLB
	 * maintenance is to be made visible to subsequent explicit
	 * memory operations.
	 *
	 * See also https://patchwork.ozlabs.org/patch/102891/



	 Catalin Marinas July 6, 2011, 4:15 p.m. | #22
	 On Wed, Jul 06, 2011 at 04:55:25PM +0100, Russell King - ARM Linux wrote:
	 > On Wed, Jul 06, 2011 at 04:52:14PM +0100, Catalin Marinas wrote:
	 > > On Tue, Jul 05, 2011 at 10:46:52AM +0100, Russell King - ARM Linux wrote:
	 > > > I've since added the dsb+isbs back to the TLB ops because the ARM ARM
	 > > > is qutie explicit that both are required to ensure that explicit
	 > > > accesses see the effect of the TLB operation.  To me it is rather
	 > > > perverse that an ISB is required to ensure that this sequence works:
	 > > >
	 > > > 	write page table entry
	 > > > 	clean
	 > > > 	dsb
	 > > > 	flush tlb entry
	 > > > 	dsb
	 > > > 	isb
	 > > > 	read/write new page
	 > >
	 > > The same requirement can be found in latest (internal only) ARM ARM as
	 > > well. I think the reason is that some memory access already in the
	 > > pipeline (but after the TLB flush) may have sampled the state of the
	 > > page table using an old TLB entry, even though the actual memory access
	 > > will happen after the DSB.
	 >
	 > It would be useful to have a statement from RG on this.  Could you
	 > pass this to him please?

	 Just checked - it is required as per the ARM ARM and there are cores
	 that rely on the ISB. As I said, it depends on the pipeline
	 implementation.


	 */

	/* The page-table walk for the read/write needs only DSB to see the
	 * changed entry, but the observing the effect of the entry over the
	 * mapped page (by program-order read/write to the page)
	 * needs ISB() in addition.
	 */
}

void mmu_init()
{
	uint32_t v;
	uintptr_t *pd;

	/* Disable the table walk for TTBR0. */
	asm volatile("mrc	p15, 0, %0, c2, c0, 2\n\t"
		     : "=r" (v));
	v |= bits_on(TTBCR_PD0);
	asm volatile("mcr	p15, 0, %0, c2, c0, 2\n\t"
		     : : "r" (v));

	/* Remove the 4MB identity map. */
	pd = (uintptr_t *)&k_pd_start;
	memset(pd, 0, 4 * sizeof(pd[0]));

	mmu_dcache_clean(pd, 4 * sizeof(uintptr_t));
	mmu_tlb_invalidate(NULL, 1024 * PAGE_SIZE);
}

int mmu_map_sections(const struct mmu_map_req *r)
{
	int i, j, k, n;
	uintptr_t va, pa, inc, *pd;
	uintptr_t de;

	inc = 1 << (map_units[r->mu] + PAGE_SIZE_SZ);

	va = (uintptr_t)r->va_start;
	pa = r->pa_start;

	n = pm_alloc_nunits[r->mu];
	pd = (uintptr_t *)&k_pd_start;
	for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
		/* Prevent overflow. */
		assert(va >= (uintptr_t)r->va_start);
		assert(pa >= r->pa_start);

		j = bits_get(va, VA_PDE_IX);

		/* Ensure that the de are invalid. */
		for (k = 0; k < n; ++k)
			assert(bits_get(pd[j + k], PDE_TYPE0) == 0);

		de = bits_set(PDE_TYPE0, 2);		/* S or SS. */
		if (r->mu == MAP_UNIT_SUPER_SECTION) {
			de |= bits_on(PDE_TYPE1);	/* SS. */
			de |= bits_push(PDE_SS_BASE, pa);
		} else {
			de |= bits_set(PDE_DOM, r->domain & 0xf);
			de |= bits_push(PDE_S_BASE, pa);
		}

		if (!r->exec)
			de |= bits_on(PDE_XN);
		if (r->shared)
			de |= bits_on(PDE_SHR);
		if (!r->global)
			de |= bits_on(PDE_NG);

		de |= bits_set(PDE_TEX, r->mt >> 2);
		de |= bits_set(PDE_C, r->mt >> 1);
		de |= bits_set(PDE_B, r->mt & 1);
		de |= bits_set(PDE_APX, r->ap >> 2);
		de |= bits_set(PDE_AP, r->ap & 3);

		for (k = 0; k < n; ++k)
			pd[j + k] = de;

		mmu_dcache_clean(&pd[j], n * sizeof(uintptr_t));

		mmu_tlb_invalidate((void *)va, inc);
	}
	return 0;
}

int mmu_map_pages(const struct mmu_map_req *r)
{
	int i, j, k, n;
	uintptr_t va, pa, inc, *pd, *pt, tpa;
	uintptr_t de, te;

	inc = 1 << (map_units[r->mu] + PAGE_SIZE_SZ);

	va = (uintptr_t)r->va_start;
	pa = r->pa_start;

	n = pm_alloc_nunits[r->mu];
	pd = (uintptr_t *)&k_pd_start;
	for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
		/* Prevent overflow. */
		assert(va >= (uintptr_t)r->va_start);
		assert(pa >= r->pa_start);

		j = bits_get(va, VA_PDE_IX);
		k = bits_get(pd[j], PDE_TYPE0);

		/* The PDE must be either empty or pointing to a PT.
		 * It must not be busy mapping section/super-section.
		 */
		assert(k == 0 || k == 1);

		/* If the PDE is empty, assign and map a new PT to map
		 * the page/large-page.
		 */
		if (k == 0) {
			lock_sched_unlock(&k_pd_lock);
			pt = mmu_slub_alloc();
			tpa = mmu_va_to_pa(pt);
			lock_sched_lock(&k_pd_lock);

			memset(pt, 0, 1024);
			mmu_dcache_clean(pt, 1024);

			de  = bits_set(PDE_TYPE0, 1);
			de |= bits_push(PDE_PT_BASE, tpa);
			pd[j] = de;
			mmu_dcache_clean(&pd[j], sizeof(uintptr_t));
		} else {
			/* k_pt is statically allocated, and manages
			 * the 4MB region starting at 0x40000000.
			 */
			if (j >= 0x400 && j < 0x404) {
				pt = (uintptr_t*)&k_pt_start;
				pt += (j - 0x400) * 0x100;
			} else {

				tpa = bits_pull(pd[j], PDE_PT_BASE);
				pt = mmu_slub_pa_to_va(tpa);
			}
		}

		pt = &pt[bits_get(va, VA_PTE_IX)];

		/* Ensure that the PTEs are invalid. */
		for (k = 0; k < n; ++k)
			assert(bits_get(pt[k], PTE_TYPE) == 0);

		if (r->mu == MAP_UNIT_LARGE_PAGE) {
			te  = bits_set(PTE_TYPE, 1);
			te |= bits_push(PTE_LP_BASE, pa);
			if (!r->exec)
				te |= bits_on(PTE_LP_XN);
			te |= bits_set(PTE_LP_TEX, r->mt >> 2);
		} else {
			te  = bits_set(PTE_TYPE, 2);
			te |= bits_push(PTE_SP_BASE, pa);
			if (!r->exec)
				te |= bits_on(PTE_SP_XN);
			te |= bits_set(PTE_SP_TEX, r->mt >> 2);
		}

		if (r->shared)
			te |= bits_on(PTE_SHR);
		if (!r->global)
			te |= bits_on(PTE_NG);

		te |= bits_set(PTE_C, r->mt >> 1);
		te |= bits_set(PTE_B, r->mt & 1);
		te |= bits_set(PTE_APX, r->ap >> 2);
		te |= bits_set(PTE_AP, r->ap & 3);

		for (k = 0; k < n; ++k)
			pt[k] = te;

		mmu_dcache_clean(pt, n * sizeof(uintptr_t));

		mmu_tlb_invalidate((void *)va, inc);
	}
	return 0;
}

int mmu_map(const struct mmu_map_req *r)
{
	int ret;
	uintptr_t mask, va, pa, inc;

	assert(r && r->n > 0);
	assert(r->mt < MT_MAX);
	assert(r->ap < AP_MAX);
	assert(r->mu < MAP_UNIT_MAX);

	lock_sched_lock(&k_pd_lock);

	/* The addresses must be aligned corresponding to the unit
	 * requested.
	 */

	inc = 1 << (map_units[r->mu] + PAGE_SIZE_SZ);
	mask = inc - 1;

	va = (uintptr_t)r->va_start;
	pa = r->pa_start;
	assert((va & mask) == 0);
	assert((pa & mask) == 0);


	if (r->mu == MAP_UNIT_SECTION || r->mu == MAP_UNIT_SUPER_SECTION)
		ret = mmu_map_sections(r);
	else
		ret = mmu_map_pages(r);
	lock_sched_unlock(&k_pd_lock);

	return ret;
}

int mmu_unmap(const struct mmu_map_req *r)
{
	int i, j, k, n;
	const int nunits[4] = {1, 16, 1, 16};
	uintptr_t mask, va, pa, inc, *pd, *pt;

	assert(r && r->n > 0);
	assert(r->mu < MAP_UNIT_MAX);

	lock_sched_lock(&k_pd_lock);
	pd = (uintptr_t *)&k_pd_start;

	/* The addresses must be aligned corresponding to the unit
	 * requested.
	 */
	inc = 1 << (map_units[r->mu] + PAGE_SIZE_SZ);
	mask = inc - 1;

	va = (uintptr_t)r->va_start;
	pa = r->pa_start;
	assert((va & mask) == 0);
	assert((pa & mask) == 0);

	n = nunits[r->mu];

	if (r->mu == MAP_UNIT_SECTION || r->mu == MAP_UNIT_SUPER_SECTION) {
		for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
			/* Prevent overflow. */
			assert(va >= (uintptr_t)r->va_start);
			assert(pa >= r->pa_start);

			mmu_tlb_invalidate((void *)va, inc);

			j = bits_get(va, VA_PDE_IX);

			/* TODO: dec refcount on the frames. */

			memset(&pd[j], 0, n * sizeof(uintptr_t));

			mmu_dcache_clean(&pd[j], n * sizeof(uintptr_t));
		}
	} else {
		for (i = 0; i < r->n; ++i, va += inc, pa += inc) {
			/* Prevent overflow. */
			assert(va >= (uintptr_t)r->va_start);
			assert(pa >= r->pa_start);

			mmu_tlb_invalidate((void *)va, inc);

			j = bits_get(va, VA_PDE_IX);
			k = bits_get(pd[j], PDE_TYPE0);

			/* The PDE must point to a PT. */
			assert(k == 1);
			if (j >= 0x400 && j < 0x404) {
				pt = (uintptr_t*)&k_pt_start;
				pt += (j - 0x400) * 0x100;
			} else {
				pa = bits_pull(pd[j], PDE_PT_BASE);
				pt = mmu_slub_pa_to_va(pa);
			}

			pt = &pt[bits_get(va, VA_PTE_IX)];

			/* TODO: dec refcount on the frames. */

			memset(pt, 0, n * sizeof(uintptr_t));

			/* TODO: check if an entire PT is now empty. If so,
			 * free it too.
			 */
			mmu_dcache_clean(pt, n * sizeof(uintptr_t));
		}
	}
	lock_sched_unlock(&k_pd_lock);
	return 0;
}

uintptr_t mmu_va_to_pa(const void *p)
{
	int i, j;
	const uintptr_t *pd;
	uintptr_t va, pa, *pt;

	lock_sched_lock(&k_pd_lock);
	pd = (uintptr_t *)&k_pd_start;

	va = (uintptr_t)p;

	i = bits_get(va, VA_PDE_IX);
	j = bits_get(pd[i], PDE_TYPE0);

	assert(j == 1 || j == 2);

	if (j == 2) {
		j = bits_get(pd[i], PDE_TYPE1);
		if (j == 0) {
			pa = bits_pull(pd[i], PDE_S_BASE);
			pa += va & ~bits_mask_shifted(PDE_S_BASE_POS,
						      PDE_S_BASE_SZ);
		} else {
			pa = bits_pull(pd[i], PDE_SS_BASE);
			pa += va & ~bits_mask_shifted(PDE_SS_BASE_POS,
						      PDE_SS_BASE_SZ);
		}
		goto exit;
	}

	if (i >= 0x400 && i < 0x404) {
		pt = (uintptr_t*)&k_pt_start;
		pt += (i - 0x400) * 0x100;
	} else {

		pa = bits_pull(pd[i], PDE_PT_BASE);
		pt = mmu_slub_pa_to_va(pa);
	}

	pt = &pt[bits_get(va, VA_PTE_IX)];

	j = bits_get(pt[0], PTE_TYPE);
	assert(j);

	if (j == 1) {
		pa = bits_pull(pt[0], PTE_LP_BASE);
		pa += va & ~bits_mask_shifted(PTE_LP_BASE_POS,
					      PTE_LP_BASE_SZ);
	} else {
		pa = bits_pull(pt[0], PTE_SP_BASE);
		pa += va & ~bits_mask_shifted(PTE_SP_BASE_POS,
					      PTE_SP_BASE_SZ);
	}
exit:
	lock_sched_unlock(&k_pd_lock);
	return pa;
}
