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
#include <irq.h>
#include <mmu.h>
#include <sched.h>
#include <slub.h>
#include <string.h>

static struct list_head threads;
struct thread *curr;
static struct thread *next;

static struct thread *sched_next()
{
	struct list_head *e;
	struct thread *t;

	list_for_each(e, &threads) {
		t = list_entry(e, struct thread, entry);
		if (t->state == THRD_STATE_READY)
			return t;
	}
	return NULL;
}

/* Runs under the timer's soft IRQ. */
void sched_timer_tick(int ticks)
{
	curr->ticks -= ticks;
}

/* Runs under SVC mode with IRQs disabled. Does not race with
 * sched_timer_tick. If sched_timer_tick is running, the irq depth
 * has to be 1 (because that is where the soft IRQs run). If a
 * timer IRQ arrives, the irq depth, being > 1, forbids another
 * instance of soft IRQ handler from running.
 *
 * This and other functions do need to serialize access to the
 * threads list.
 */
char sched_should_switch()
{
	/* THRD_STATE_EVICTING ==
	 * Waiting for the switch routine to run. It runs
	 * from excpt_irq_addr, once all soft IRQs are done
	 * and the function switches back to the IRQ mode
	 * with IRQs disabled.
	 */

	if (curr->ticks > 0 || !curr->preemptible)
		return 0;

	next = sched_next();
	if (next == NULL) {
		curr->ticks = curr->quota;
		return 0;
	}

	return 1;
}

/* Runs under SVC mode with IRQs disabled. */
void *swtch(void *outf)
{
	struct thread *t;

	t = curr;
	curr = next;

	/* Barrier to change the values after curr has been
	 * reassigned.
	 */

	t->context = outf;
	t->state = THRD_STATE_READY;

	list_del(&t->entry);
	list_add_tail(&t->entry, &threads);

	curr->ticks = curr->quota;
	curr->state = THRD_STATE_RUNNING;
	return curr->context;
}

void sched_init()
{
	extern char stack_hi;
	struct thread *t;

	init_list_head(&threads);

	t = kmalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->preemptible = 1;
	t->quota = THRD_QUOTA;
	t->ticks = t->quota;
	t->state = THRD_STATE_RUNNING;
	t->svc_stack_hi = &stack_hi;
	list_add(&t->entry, &threads);

	curr = t;
}

struct thread *sched_thread_create(thread_fn fn, void *data)
{
	struct thread *t;
	uintptr_t *p;

	t = kmalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->preemptible = 1;
	t->quota = THRD_QUOTA;
	t->ticks = t->quota;
	t->state = THRD_STATE_READY;
	t->svc_stack_hi = kmalloc(PAGE_SIZE) + PAGE_SIZE;

	p = (uintptr_t *)t->svc_stack_hi;
	--p;
	*p = 0x153;
	--p;
	*p = (uintptr_t)fn;
	--p;
	*p = 0;

	p -= 5;
	*p = (uintptr_t)data;
	p -= 8;

	t->context = p;

	/* r0 must be set to p.
	 * lr_irq must be set to fn.
	 * spsr_irq.mode must be SVC.
	 */

	/* Barrier. */
	list_add_tail(&t->entry, &threads);
	return t;
}
