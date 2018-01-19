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

static struct list_head ready;
struct thread *current;

/* Runs under the timer's soft IRQ. */
void sched_timer_tick(int ticks)
{
	current->ticks -= ticks;
}

int sched_quota_consumed()
{
	int ret;

	ret = 0;

	/* Sync with the timer soft IRQ. */
	irq_soft_disable();
	if (current->ticks <= 0)
		ret = 1;
	irq_soft_enable();

	return ret;
}

void sched_switch(void **ctx)
{
	struct thread *next;

	next = container_of(ctx, struct thread, context);

	/* This is the point at which the new thread starts accumulating
	 * ticks.
	 */
	irq_soft_disable();
	current = next;
	irq_soft_enable();
}

/* Can be called from schedule() or from excpt.S. */
static void schedule_preempt_disabled()
{
	void *ctx;
	struct list_head *e;
	struct thread *next;
	extern void *_schedule(void **, void **);

	if (current->state == THRD_STATE_WAKING)
		return;

	if (list_empty(&ready)) {
		irq_soft_disable();
		/* If this was a voluntary schedule and no ready
		 * thread was found, do not reset the quota.
		 */
		if (current->ticks <= 0)
			current->ticks = THRD_QUOTA;
		irq_soft_enable();
		return;
	}

	e = ready.next;
	list_del(e);

	next = list_entry(e, struct thread, entry);

	/* If the next thread had voluntary scheduled, do
	 * not reset the quota.
	 */
	if (next->ticks <= 0)
		next->ticks = THRD_QUOTA;

	/* Because of the soft IRQ, the ready current thread may
	 * continue accumulating ticks.
	 */
	if (current->state == THRD_STATE_READY)
		list_add_tail(&current->entry, &ready);

	/* Pass the pointer to the context field so that
	 * _schedule can save the stack pointer to the context frame
	 * into it.
	 */
	ctx = &current->context;

	/* The function switches the stack upon return. That creates
	 * problems for a newly-created thread, since it needs to
	 * reserve space. We reserve 8 words.
	 */
	ctx = _schedule(&next->context, ctx);

	/* Do not use any local variable initialized from above the
	 * call to _schedule. Those values are stale.
	 */
	sched_switch(ctx);
}

/* Preemption must be enabled when schedule is called. */
void schedule()
{
	preempt_disable();
	assert(preempt_depth() == 1);
	schedule_preempt_disabled();
	preempt_enable();
}

void wake_up(struct list_head *wq)
{
	struct list_head *e;
	struct thread *t;

	preempt_disable();
	while (!list_empty(wq)) {
		e = wq->next;
		list_del(e);

		t = list_entry(e, struct thread, entry);
		t->state = THRD_STATE_WAKING;
		list_add_tail(e, &ready);
	}
	preempt_enable();
}

void sched_init()
{
	extern char stack_hi;
	struct thread *t;

	init_list_head(&ready);

	t = kmalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->ticks = THRD_QUOTA;
	t->state = THRD_STATE_RUNNING;
	t->svc_stack_hi = &stack_hi;

	current = t;
}

struct thread *sched_thread_create(thread_fn fn, void *data)
{
	struct thread *t;
	struct context *ctx;

	t = kmalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));

	t->ticks = THRD_QUOTA;
	t->state = THRD_STATE_READY;
	t->svc_stack_hi = kmalloc(PAGE_SIZE) + PAGE_SIZE;

	ctx = t->svc_stack_hi - sizeof(*ctx);
	ctx->is_fresh = 1;
	ctx->cpsr = 0x153;
	ctx->reg[0] = (uintptr_t)data;
	ctx->lr = (uintptr_t)fn;
	t->context = ctx;

	preempt_disable();
	list_add_tail(&t->entry, &ready);
	preempt_enable();

	return t;
}
