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
#include <irq.h>

extern char vm_vec_start;
static const void *vec_base = &vm_vec_start;
static void loop()
{
	while (1)
		asm volatile("wfi");
}

void irq_vec_reset() {loop();}
void irq_vec_undef() {loop();}
void irq_vec_svc() {loop();}
void irq_vec_pabort() {loop();}
void irq_vec_dabort() {loop();}
void irq_vec_res() {loop();}
void irq_vec_irq() {loop();}
void irq_vec_fiq() {loop();}

void irq_init()
{
	int ret;
	uintptr_t pa;
	struct mmu_map_req r;
	extern char irq_vector;

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_NORMAL, 1, &pa);
	assert(ret == 0);

	r.va_start = (void *)vec_base;
	r.pa_start = pa;
	r.n = 1;
	r.mt = MT_NRM_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	memcpy(r.va_start, &irq_vector, 16 * 4);

	/* The writes must be cleaned to a point visible to
	 * instruction fetch, which is usually L2 or RAM.
	 */
	mmu_dcache_clean(r.va_start, 16 * 4);

	ret = mmu_unmap(NULL, &r);
	assert(ret == 0);

	/* Invalidate the TLB entries since the page will be
	 * remapped with different parameters.
	 */
	mmu_tlb_invalidate(r.va_start, 16 * 4);

	r.ap = AP_SRO;
	r.exec = 1;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	/* TODO Drain pipeline. */

	/* Enable IRQs and FIQs within CPSR. */
	asm volatile("cpsie if");
}
