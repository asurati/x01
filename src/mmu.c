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

extern char KMODE_VA;
extern char pt_start;
extern char k_pd_start;

const uintptr_t kmode_va = (uintptr_t)&KMODE_VA;
const uintptr_t pt_area_va = (uintptr_t)&pt_start;

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
	int i;
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
	for (i = 0; i < 4; ++i)
		pd[i] = 0;

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
        BF_SET(te, PTE_TYPE, 1);        /* Small Page. */
        BF_SET(te, PTE_AP, 1);          /* Supervisor-only. */
        BF_SET(te, PTE_SP_XN, 1);       /* No Execute. */
        BF_SET(te, PTE_C, 1);           /* I/O WB, AoW. */
        BF_SET(te, PTE_B, 1);
        BF_SET(te, PTE_SP_TEX, 1);
        BF_PUSH(te, PTE_SP_BASE, pa);
        pt[0] = te;
	mmu_dcache_clean(pt, sizeof(uintptr_t));
}
