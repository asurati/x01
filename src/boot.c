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

struct section {
	uintptr_t start;
	uintptr_t end;
	char desc;
};

extern char text_start, text_end;
extern char rodata_start, rodata_end;
extern char data_start, data_end;
extern char bss_start, bss_end;
extern char k_pd_start, k_pd_end;
//extern char ptab_start, ptab_end;

/* rwx == 4 2 1. */
static const struct section si[] = {
	{(uintptr_t)&text_start,	(uintptr_t)&text_end,	5},
	{(uintptr_t)&rodata_start,	(uintptr_t)&rodata_end,	4},
	{(uintptr_t)&data_start,	(uintptr_t)&data_end,	6},
	{(uintptr_t)&bss_start,		(uintptr_t)&bss_end,	6},
	{(uintptr_t)&k_pd_start,	(uintptr_t)&k_pd_end,	6},
	//{(uintptr_t)&ptab_start,	(uintptr_t)&ptab_end,	6},
};

/* Zero the kernel page directory and table.
 * Identity map the first 4 MB of RAM.
 * Map the sections appropriately.
 */
void boot_map()
{
	char c;
	int i, j, sz;
	uintptr_t *pd, *pt, de, te;
	uintptr_t pa, va, kmode_va;
	extern char k_pd_pa;
	extern char k_pt_pa;
	extern char k_pta_pt_pa;
	extern char pt_start;
	extern char KMODE_VA;

	pt = (uintptr_t *)&k_pta_pt_pa;
	sz = PAGE_SIZE;
	for (i = 0; i < sz >> 2; ++i)
		pt[i] = 0;

	pt = (uintptr_t *)&k_pt_pa;
	sz = PAGE_SIZE;
	for (i = 0; i < sz >> 2; ++i)
		pt[i] = 0;

	pd = (uintptr_t *)&k_pd_pa;
	sz = 4 * PAGE_SIZE;
	for (i = 0; i < sz >> 2; ++i)
		pd[i] = 0;

	/* Identity map. To be removed once the execution switches to using
	 * virtual addresses.
	 */
	for (i = 0; i < 4; ++i) {
		de = 0;
		BF_SET(de, PDE_TYPE0, 2);	/* Section. */
		BF_SET(de, PDE_S_BASE, i);	/* Base. */
		BF_SET(de, PDE_AP, 1);		/* Supervisor-only, RW. */
		BF_SET(de, PDE_C, 1);		/* I/O WB, AoW. */
		BF_SET(de, PDE_B, 1);
		BF_SET(de, PDE_TEX, 1);
		pd[i] = de;
	}

	/* Fill in the PDEs for the 4MB PT area. */
	va = (uintptr_t)&pt_start;
	j = BF_GET(va, VA_PDE_IX);
	pa = (uintptr_t)&k_pta_pt_pa;
	for (i = 0; i < 4; ++i, pa += 0x400) {
		de = 0;
		BF_SET(de, PDE_TYPE0, 1);
		BF_PUSH(de, PDE_PT_BASE, pa);
		pd[i + j] = de;
	}

	/* The recursive mapping page VA for the current settings is
	 * 0x40902000, whose PTE is located within itself at 0x40902408.
	 * The VA 0x40902000 then correponds to the PT area PT k_pta_pt_pa.
	 * Hence, the offset 0x408 must contain the pa.
	 */
	pa = (uintptr_t)&k_pta_pt_pa;
	pt = (void *)(pa + 0x408);
	te = 0;
	BF_SET(te, PTE_TYPE, 2);	/* Small Page. */
	BF_SET(te, PTE_AP, 1);		/* Supervisor-only. */
	BF_SET(te, PTE_SP_XN, 1);	/* No Execute. */
	BF_SET(te, PTE_C, 1);		/* I/O WB, AoW. */
	BF_SET(te, PTE_B, 1);
	BF_SET(te, PTE_SP_TEX, 1);
	BF_PUSH(te, PTE_SP_BASE, pa);
	pt[0]  = te;

	/* First 4MB RAM mapped to first 4MB @ KMODE_VA using small pages. */
	kmode_va = (uintptr_t)&KMODE_VA;
	j = BF_GET(kmode_va, VA_PDE_IX);
	pa = (uintptr_t)&k_pt_pa;
	for (i = 0; i < 4; ++i, pa += 0x400) {
		de = 0;
		BF_SET(de, PDE_TYPE0, 1);	/* PT. */
		BF_PUSH(de, PDE_PT_BASE, pa);
		pd[i + j] = de;
	}

	for (i = 0; (unsigned)i < ARRAY_SIZE(si); ++i) {
		for (va = si[i].start; va < si[i].end; va += PAGE_SIZE) {
			j = BF_GET(va, VA_PDE_IX);

			pa = BF_PULL(pd[j], PDE_PT_BASE);
			pt = (void *)pa;

			te = 0;
			c = si[i].desc;

			BF_SET(te, PTE_TYPE, 2);	/* Small Page. */
			BF_SET(te, PTE_AP, 1);		/* Supervisor-only. */

			/* no execute. */
			if ((c & 1) == 0)
				BF_SET(te, PTE_SP_XN, 1);

			/* no write. */
			if ((c & 2) == 0)
				BF_SET(te, PTE_APX, 1);

			BF_SET(te, PTE_C, 1);		/* I/O WB, AoW. */
			BF_SET(te, PTE_B, 1);
			BF_SET(te, PTE_SP_TEX, 1);
			BF_PUSH(te, PTE_SP_BASE, va - kmode_va);

			j = BF_GET(va, VA_PTE_IX);
			pt[j]  = te;
		}
	}
}
