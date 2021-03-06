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
#include <slub.h>
#include <string.h>
#include <mutex.h>

#include <sys/sched.h>
#include <sys/timer.h>

static struct list_head ready;
struct thread *current;
static struct thread *idle;
static struct thread _current;

static struct list_head timer_heads[SCHED_MAX_TOUT_TICKS];
static uint32_t timer_req_mask, timer_recv_mask;
static int timer_ticks;

uint32_t rol(uint32_t v, int c)
{
	if ((c & 0x1f) == 0)
		return v;

	c = 32 - c;

	asm volatile("ror %0, %1, %2"
		     : "=r" (v)
		     : "r" (v), "r" (c));
	return v;
}

/* Runs as a function under the timer's soft IRQ. */
_ctx_soft
void sched_timer_tick_soft(int ticks)
{
	uint32_t mask;

	/* If the current thread is not the idle thread,
	 * charge the ticks.
	 */
	if (current != idle) {
		current->ticks -= ticks;
		if (current->ticks <= 0)
			irq_sched_raise(IRQ_SCHED_SCHEDULE);
	}

	if (ticks >= SCHED_MAX_TOUT_TICKS) {
		/* Too much of a delay; all timers affected. */
		mask = -1;
	} else {
		mask = (1 << ticks) - 1;
		mask = rol(mask, timer_ticks + 1);
	}

	mask &= timer_req_mask;

	if (mask) {
		/* Accumulate the timers which need to be fired. */
		timer_recv_mask |= mask;
		irq_sched_raise(IRQ_SCHED_TIMER);
	}

	timer_ticks += ticks;
	timer_ticks &= SCHED_MAX_TOUT_TICKS_MASK;
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

_ctx_proc
int schedule()
{
	int ret;

	ret = preempt_disable();
	assert(ret == 1);
	ret = schedule_preempt_disabled();
	preempt_enable();
	return ret;
}

_ctx_sched
void wake_up_preempt_disabled(struct list_head *wq)
{
	int queue_sched_irq;
	struct list_head *e;
	struct thread *t;

	queue_sched_irq = 0;

	while (!list_empty(wq)) {
		e = wq->next;
		list_del(e);

		t = list_entry(e, struct thread, entry);

		assert(t != idle);

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
			queue_sched_irq = 1;
		}
	}

	if (current == idle && queue_sched_irq)
		irq_sched_raise(IRQ_SCHED_SCHEDULE);
}

_ctx_proc
void wake_up(struct list_head *wq)
{
	assert(preempt_disable() == 1);
	wake_up_preempt_disabled(wq);
	preempt_enable();
}

_ctx_sched
static int sched_irq_schedule(void *data)
{
	(void)data;
	return schedule_preempt_disabled();
}

_ctx_proc
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

_ctx_proc
static int sched_idle(void *data)
{
	(void)data;

	while (1)
		wfi();

	return 0;
}

_ctx_proc
void mutex_init(struct mutex *m)
{
	init_list_head(&m->wq);
}

_ctx_proc
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

_ctx_proc
void mutex_unlock(struct mutex *m)
{
	preempt_disable();
	m->lock = 0;
	wake_up_preempt_disabled(&m->wq);
	preempt_enable();
}

/* Timers fire at _ctx_sched level, before any threads
 * get to run. That ensures that the queue is not modified between the time
 * the IRQ_SCHED_TIMER is raised and the time the function runs.
 */
_ctx_soft
_ctx_sched
static int sched_irq_timer(void *data)
{
	int i;
	uint32_t mask;
	struct list_head *e, *h;
	struct timer *t;

	(void)data;

	irq_soft_disable();
	mask = timer_recv_mask;
	/* Reset the recv mask. */
	timer_recv_mask = 0;
	/* Turn off the bits for the timers which are about to be fired. */
	timer_req_mask &= ~mask;
	irq_soft_enable();


	if (mask == 0)
		return 0;

	/* The timer routines run at _ctx_sched. */

	/* TODO Utilize FFS. */
	for (i = 0; i < 32; ++i) {
		if ((mask & (1 << i)) == 0)
			continue;
		h = &timer_heads[i];
		assert(!list_empty(h));
		while (!list_empty(h)) {
			e = h->next;
			list_del(e);
			t = list_entry(e, struct timer, entry);
			t->fn(t->data);
		}
	}
	return 0;
}

_ctx_sched
static int sched_timer_wakeup(void *data)
{
	struct timer *t;
	t = data;
	wake_up_preempt_disabled(&t->wq);
	return 0;
}

_ctx_proc
void msleep(int ms)
{
	int ticks, ft;
	struct timer t;

	assert(ms > 0);

	ticks = (HZ * ms) / 1000;
	if (ticks == 0)
		ticks = 1;

	/* Overflow. */
	assert(ticks > 0);

	/* Max supported wait time is 32 ticks. */
	assert(ticks <= SCHED_MAX_TOUT_TICKS);

	init_list_head(&t.wq);
	t.fn = sched_timer_wakeup;
	t.data = &t;

	/* Although the timer routines run at _ctx_sched, the queueing
	 * needs to be done at soft IRQ level since the list is
	 * indirectly controlled by the scheduler's timer soft IRQ
	 * routine.
	 */
	irq_soft_disable();
	ft = timer_ticks + ticks;
	ft &= SCHED_MAX_TOUT_TICKS_MASK;
	list_add_tail(&t.entry, &timer_heads[ft]);
	timer_req_mask |= 1 << ft;
	irq_soft_enable();

	/* The wakeup could arrive in between the queuing of the timer,
	 * and going for a wait. wait_event() handles the situation.
	 */
	wait(&t.wq);
}

_ctx_init
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

_ctx_init
void sched_init()
{
	int i;

	init_list_head(&ready);
	for (i = 0; i < SCHED_MAX_TOUT_TICKS; ++i)
		init_list_head(&timer_heads[i]);

	irq_sched_insert(IRQ_SCHED_SCHEDULE, sched_irq_schedule, NULL);
	irq_sched_insert(IRQ_SCHED_TIMER, sched_irq_timer, NULL);

	idle = sched_thread_create(sched_idle, NULL);
}
