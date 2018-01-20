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
void excpt_irq()
{
	extern int irq_hard();
	extern int irq_soft();
	extern int sched_quota_consumed();
	extern void schedule();

	irq_enter();

	irq_hard();

	irq_enable();

	if (irq_depth() > 1 || irq_soft_depth() > 0)
		goto irq_done;

	irq_soft();

	if (preempt_depth() == 0 && sched_quota_consumed()) {
		set_current_state(THRD_STATE_READY);
		schedule();
		set_current_state(THRD_STATE_RUNNING);
	}

irq_done:
	irq_disable();
	irq_exit();
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