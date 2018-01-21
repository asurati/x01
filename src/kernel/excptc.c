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
#include <irq.h>
#include <sched.h>

extern char excpt_start;
static const void *excpt_vec_base = &excpt_start;

static void loop()
{
	while (1)
		asm volatile("wfi");
}

void excpt_reset() {loop();}
void excpt_undef() {loop();}
void excpt_svc() {loop();}
void excpt_pabort() {loop();}
void excpt_dabort() {loop();}
void excpt_res() {loop();}
void excpt_fiq() {loop();}

/* Called with IRQs disabled. */
_ctx_hard
void excpt_irq()
{
	int depth;
	extern int irq_hard();
	extern int irq_soft();
	extern int irq_sched();

	irq_hard();

	depth = irq_soft_disable();

	/* Within a recursive context, or soft IRQs disabled
	 * by the thread.
	 */
	if (depth > 1)
		goto irq_no_soft;

	set_current_irq_ctx(1);
	/* Within the primary IRQ context, with soft IRQs enabled. */
	irq_enable();

	irq_soft();

	/* Do not enable soft IRQs yet, else recursive contexts will
	 * end up calling irq_soft() while primary context is still
	 * within excpt_irq().
	 */

	depth = irq_sched_disable();

	/* Within the primary IRQ countext, with sched IRQs disabled. */
	if (depth > 1)
		goto irq_no_sched;

	irq_sched();

irq_no_sched:
	irq_sched_enable();
	irq_disable();
	set_current_irq_ctx(0);
irq_no_soft:
	irq_soft_enable();
}

void excpt_init()
{
	int ret;
	struct mmu_map_req r;
	extern char excpt_start_pa;

	r.va_start = (void *)excpt_vec_base;
	r.pa_start = (uintptr_t)&excpt_start_pa;
	r.n = 1;
	r.mt = MT_NRM_WBA;
	r.ap = AP_SRO;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 1;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	/* TODO Drain pipeline. */
}
