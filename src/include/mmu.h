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

#ifndef _MMU_H_
#define _MMU_H_

#include <types.h>

extern const uintptr_t kmode_va;
extern const uintptr_t pt_area_va;

#define CACHE_LINE_SIZE_SZ	5
#define CACHE_LINE_SIZE		(1ull << CACHE_LINE_SIZE_SZ)
#define CACHE_LINE_MASK		BF_MASK(CACHE_LINE_SIZE)

#define PAGE_SIZE_SZ		12
#define PAGE_SIZE		(1ull << PAGE_SIZE_SZ)
#define PAGE_SIZE_MASK		BF_MASK(PAGE_SIZE)

#define PDE_TYPE0_POS		 0
#define PDE_B_POS		 2
#define PDE_C_POS		 3
#define PDE_XN_POS		 4
#define PDE_DOM_POS		 5
#define PDE_AP_POS		10
#define PDE_TEX_POS		12
#define PDE_APX_POS		15
#define PDE_SHR_POS		16
#define PDE_NG_POS		17
#define PDE_TYPE1_POS		18
#define PDE_NS_POS		19
#define PDE_PT_BASE_POS		10
#define PDE_S_BASE_POS		20
#define PDE_SS_BASE_POS		24
#define PDE_PT_NS_POS		 3

#define PDE_TYPE0_SZ		 2
#define PDE_B_SZ		 1
#define PDE_C_SZ		 1
#define PDE_XN_SZ		 1
#define PDE_DOM_SZ		 4
#define PDE_AP_SZ		 2
#define PDE_TEX_SZ		 3
#define PDE_APX_SZ		 1
#define PDE_SHR_SZ		 1
#define PDE_NG_SZ		 1
#define PDE_TYPE1_SZ		 1
#define PDE_NS_SZ		 1
#define PDE_PT_BASE_SZ		22
#define PDE_S_BASE_SZ		12
#define PDE_SS_BASE_SZ		 8
#define PDE_PT_NS_SZ		 1

#define PTE_SP_XN_POS		 0
#define PTE_LP_XN_POS		15
#define PTE_TYPE_POS		 1
#define PTE_B_POS		 2
#define PTE_C_POS		 3
#define PTE_AP_POS		 4
#define PTE_SP_TEX_POS		 6
#define PTE_LP_TEX_POS		12
#define PTE_APX_POS		 9
#define PTE_SHR_POS		10
#define PTE_NG_POS		11
#define PTE_SP_BASE_POS		12
#define PTE_LP_BASE_POS		16

#define PTE_SP_XN_SZ		 1
#define PTE_LP_XN_SZ		 1
#define PTE_TYPE_SZ		 1
#define PTE_B_SZ		 1
#define PTE_C_SZ		 1
#define PTE_AP_SZ		 2
#define PTE_SP_TEX_SZ		 3
#define PTE_LP_TEX_SZ		 3
#define PTE_APX_SZ		 1
#define PTE_SHR_SZ		 1
#define PTE_NG_SZ		 1
#define PTE_SP_BASE_SZ		20
#define PTE_LP_BASE_SZ		16

#define VA_PDE_IX_POS		20
#define VA_PTE_IX_POS		12
#define VA_PDE_IX_SZ		12
#define VA_PTE_IX_SZ		 8

#define TTBCR_PD0_POS		 4
#define TTBCR_PD0_SZ		 1

#ifdef QRPI2

#define mmu_dcache_clean mmu_dcache_clean_nomcrr
void mmu_dcache_clean_nomcrr(void *va, size_t sz);

#else /* QRPI2 */

#define mmu_dcache_clean mmu_dcache_clean_mcrr
void mmu_dcache_clean_mcrr(void *va, size_t sz);

#endif /* QRPI2 */

#define mmu_dsb() \
	do { \
		asm volatile("mcr	p15, 0, %0, c7, c10, 4\n\t" \
			     : : "r" (0)); \
	} while (0)

void mmu_init();
void mmu_dcache_clean(void *va, size_t sz);
void mmu_tlb_invalidate(void *va, size_t sz);
#endif
