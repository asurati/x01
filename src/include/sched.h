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
#define THRD_STATE_MUTEX_WAITING	5

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
	char in_irq_ctx;
	char res[1];
	int irq_soft_count;
	int irq_sched_count;
};

extern struct thread *current;

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

static inline int irq_soft_disable()
{
	*(volatile int *)(&current->irq_soft_count) += 1;
	barrier();
	return *(volatile int *)(&current->irq_soft_count);
}

static inline void irq_soft_enable()
{
	int v;
	extern int irq_soft();

	barrier();
	v = *(volatile int *)(&current->irq_soft_count);

	if (!current->in_irq_ctx && v == 1)
		irq_soft();

	*(volatile int *)(&current->irq_soft_count) -= 1;
}

static inline int irq_sched_disable()
{
	*(volatile int *)(&current->irq_sched_count) += 1;
	barrier();
	return *(volatile int *)(&current->irq_sched_count);
}

static inline void irq_sched_enable()
{
	int v;
	extern int irq_sched();

	barrier();
	v = *(volatile int *)(&current->irq_sched_count);
	if (!current->in_irq_ctx && v == 1)
		irq_sched();

	*(volatile int *)(&current->irq_sched_count) -= 1;
}

#define preempt_disable()		irq_sched_disable()
#define preempt_enable()		irq_sched_enable()

#define wait(wq)							\
	do {								\
		preempt_disable();					\
		set_current_state(THRD_STATE_WAITING);			\
		list_add_tail(&current->entry, (wq));			\
		preempt_enable();					\
		schedule();						\
	} while (0)

#define wait_event(wq, cond)						\
	do {								\
		preempt_disable();					\
		if (cond) {						\
			preempt_enable();				\
			break;						\
		}							\
		set_current_state(THRD_STATE_WAITING);			\
		list_add_tail(&current->entry, (wq));			\
		preempt_enable();					\
		while (!(cond)) {					\
			schedule();					\
		}							\
	} while (0)

typedef int (*thread_fn)(void *p);

struct thread	*sched_thread_create(thread_fn fn, void *p);
void		wake_up(struct list_head *wq);
int		schedule();
void		msleep(int ms);
#endif
