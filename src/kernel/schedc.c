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
#include <mutex.h>

static struct list_head ready;
struct thread *current;
static struct thread *idle;
static struct thread _current;

/* Runs under the timer's soft IRQ. */

_ctx_soft
void sched_timer_tick(int ticks)
{
	if (current == idle)
		return;

	current->ticks -= ticks;
	if (current->ticks <= 0)
		irq_sched_raise(IRQ_SCHED_SCHED);
}

_ctx_sched
void sched_switch(void **ctx)
{
	struct thread *next;

	next = container_of(ctx, struct thread, context);

	/* If the next thread had quota left over, do not reset it. */
	if (next->ticks <= 0)
		next->ticks = THRD_QUOTA;

	/* This is the point at which the new thread starts accumulating
	 * ticks. The changes made to the fields of next must be visible
	 * before the current is set to next. Else, it may so happen that
	 * although the current is updated to the next, accessing those
	 * fields through the current pointer may still provide older
	 * values (values from when the next was kept on the ready queue).
	 *
	 * The barrier within the soft IRQ lock ensures that the changes
	 * are visible in the correct order.
	 */
	next->state = THRD_STATE_RUNNING;
	current = next;
}

/* Can be called from schedule() or sched_irq(), with preemption
 * disabled.
 */
_ctx_sched
static int schedule_preempt_disabled()
{
	int empty;
	void *ctx;
	struct list_head *e;
	struct thread *next;
	extern void *_schedule(void **, void **);

	/* An idle thread is either running or is on the ready queue.
	 * If the ready queue is empty, the running thread must be
	 * the idle thread.
	 */
	if (list_empty(&ready)) {
		assert(current == idle);
		return SCHED_RET_RUN;
	}

	/* The queue is not empty. */
	e = ready.next;
	list_del(e);
	next = list_entry(e, struct thread, entry);
	assert(current != next);

	if (next == idle) {
		/* We just removed the idle thread from the queue.
		 * If the queue is now empty, and the current thread wants
		 * to continue running, allow it. Place the idle thread
		 * back into the end of the queue.
		 */
		empty = list_empty(&ready);

		/* If the queue did not become empty when the idle thread
		 * was removed, then there is another thread available to
		 * switch to. Queue the idle back at the end, and pick the
		 * next available.
		 */
		if (!empty) {
			list_add_tail(e, &ready);

			e = ready.next;
			list_del(e);
			next = list_entry(e, struct thread, entry);

			assert(current != next);
			assert(next != idle);
		} else if (current->state == THRD_STATE_RUNNING) {
			assert(current != idle);
			/* The queue became empty upon the removal of the idle
			 * thread. Moreover, the current thread wishes to
			 * continue to run. Allow it, and place idle back into
			 * the end.
			 */
			list_add_tail(e, &ready);
			if (current->ticks <= 0)
				current->ticks = THRD_QUOTA;
			return SCHED_RET_RUN;
		} else {
			/* The queue became empty upon the removal of the idle
			 * thread. Moreover, the current thread does not wish
			 * to continue to run. Switch to idle.
			 */
		}
	}

	/* Because of the soft IRQ, the ready current thread may
	 * continue accumulating ticks.
	 */
	if (current->state == THRD_STATE_RUNNING) {
		set_current_state(THRD_STATE_READY);
		list_add_tail(&current->entry, &ready);
	}

	/* Pass the pointer to the context field so that
	 * _schedule can save the stack pointer to the context frame
	 * into it.
	 */
	ctx = &current->context;

	/* The function switches the stack upon return. */
	ctx = _schedule(&next->context, ctx);

	/* Do not use any local variable initialized from above the
	 * call to _schedule. Those values are stale.
	 */
	sched_switch(ctx);
	return SCHED_RET_SWITCH;
}

/* Preemption must be enabled when schedule is called. */
int schedule()
{
	int ret;

	ret = preempt_disable();
	assert(ret == 1);
	ret = schedule_preempt_disabled();
	preempt_enable();
	return ret;
}

void wake_up_preempt_disabled(struct list_head *wq)
{
	struct list_head *e;
	struct thread *t;

	while (!list_empty(wq)) {
		e = wq->next;
		list_del(e);

		t = list_entry(e, struct thread, entry);

		assert(t != idle);
		assert(current != idle);

		assert(t->state == THRD_STATE_WAITING ||
		       t->state == THRD_STATE_MUTEX_WAITING);
		/* t == current is possible when the ready queue is empty.
		 * When the queue is empty, the current thread, though
		 * marked as waiting on a wait queue, is allowed to run
		 */
		if (t == current) {
			t->state = THRD_STATE_RUNNING;
		} else {
			t->state = THRD_STATE_READY;
			list_add_tail(e, &ready);
		}
	}
}

void wake_up(struct list_head *wq)
{
	preempt_disable();
	wake_up_preempt_disabled(wq);
	preempt_enable();
}

_ctx_sched
static int sched_irq(void *data)
{
	data = data;
	return schedule_preempt_disabled();
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

static int sched_idle(void *data)
{
	data = data;

	while (1)
		asm volatile("wfi");

	return 0;
}

void mutex_init(struct mutex *m)
{
	init_list_head(&m->wq);
}

void mutex_lock(struct mutex *m)
{
	preempt_disable();
	while (m->lock == 1) {
		set_current_state(THRD_STATE_MUTEX_WAITING);
		list_add_tail(&current->entry, &m->wq);
		preempt_enable();
		schedule();
		preempt_disable();
	}
	m->lock = 1;
	preempt_enable();

	/* preempt_enable() provides release semantics. */
}

void mutex_unlock(struct mutex *m)
{
	preempt_disable();
	m->lock = 0;
	barrier();
	wake_up_preempt_disabled(&m->wq);
	preempt_enable();
}

void sched_current_init()
{
	extern char stack_hi;
	struct thread *t;

	t = &_current;

	memset(t, 0, sizeof(*t));

	t->ticks = THRD_QUOTA;
	t->state = THRD_STATE_RUNNING;
	t->svc_stack_hi = &stack_hi;
	current = t;
}

void sched_init()
{
	init_list_head(&ready);
	irq_sched_insert(IRQ_SCHED_SCHED, sched_irq, NULL);

	idle = sched_thread_create(sched_idle, NULL);
}
