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

#ifndef _SCHED_H_
#define _SCHED_H_

#include <list.h>
#include <barrier.h>

#define THRD_STATE_RUNNING		1
#define THRD_STATE_READY		2
#define THRD_STATE_WAITING		3
#define THRD_STATE_WAKING		4

#define THRD_QUOTA			5

/* thread.ticks: Accesses need sync with soft IRQs.
 * thread.state: Accesses need sync with scheduler.
 */
struct thread {
	struct list_head entry;
	void *usr_stack_hi;
	void *svc_stack_hi;
	void *context;
	char ticks;
	char state;
	char res[2];
	int preempt_disable_depth;
	int irq_soft_disable_depth;
	int irq_disable_depth;
};

extern struct thread *current;

struct context {
	uintptr_t is_fresh;
	uintptr_t cpsr;
	uintptr_t reg[13];
	uintptr_t lr;
};

/* Should we add barrier here? */
#define set_current_state(s)						\
	do {								\
		current->state = (s);					\
	} while (0)

/* Disable/Enable provide acquire/release semantics on
 * single processor alone. The effects of the
 * enable/disable operations are observed only on the
 * processor on which the thread is currently running.
 * Hence, hardware barriers are not needed here.
 *
 * For any scheduler fields which can potentially be
 * accessed by multiple agents, hardware barriers are
 * required.
 */
static inline void preempt_disable()
{
	*(volatile int *)(&current->preempt_disable_depth) += 1;
	barrier();
}

static inline void preempt_enable()
{
	barrier();
	*(volatile int *)(&current->preempt_disable_depth) -= 1;
}

static inline int preempt_depth()
{
	return *(volatile int *)(&current->preempt_disable_depth);
}

static inline void irq_soft_disable()
{
	*(volatile int *)(&current->irq_soft_disable_depth) += 1;
	barrier();
}

static inline void irq_soft_enable()
{
	barrier();
	*(volatile int *)(&current->irq_soft_disable_depth) -= 1;
}

static inline int irq_soft_depth()
{
	return *(volatile int *)(&current->irq_soft_disable_depth);
}

/* These irq routines are meant to be called from excpt.S only.
 * They are called with IRQs disabled. */
static inline void irq_enter()
{
	*(volatile int *)(&current->irq_disable_depth) += 1;
	barrier();
}

static inline void irq_exit()
{
	barrier();
	*(volatile int *)(&current->irq_disable_depth) -= 1;
}

static inline int irq_depth()
{
	return *(volatile int *)(&current->irq_disable_depth);
}

/* Disabling soft IRQs imply disabling preemption. */
static inline int preempt_disabled()
{
	return (irq_depth() > 0) ||
		(irq_soft_depth() > 0) ||
		(preempt_depth() > 0);
}

#define wait_event(wq, cond)						\
	do {								\
		preempt_disable();					\
		set_current_state(THRD_STATE_WAITING);			\
		list_add_tail(&current->entry, (wq));			\
		while (!(cond)) {					\
			preempt_enable();				\
			schedule();					\
			preempt_disable();				\
		}							\
		set_current_state(THRD_STATE_RUNNING);			\
		preempt_enable();					\
	} while (0)

typedef int (*thread_fn)(void *p);

void		sched_init();
void		sched_timer_tick();
struct thread	*sched_thread_create(thread_fn fn, void *p);
void		wake_up(struct list_head *wq);
void		schedule();
#endif
