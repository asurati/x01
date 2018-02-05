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

#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <list.h>

#define THRD_QUOTA				5

#define	SCHED_RET_SWITCH			1
#define	SCHED_RET_RUN				2

struct context {
	uintptr_t is_fresh;
	uintptr_t cpsr;
	uintptr_t reg[13];
	uintptr_t lr;
};

#define SCHED_MAX_TOUT_TICKS			32
#define SCHED_MAX_TOUT_TICKS_MASK					\
	(SCHED_MAX_TOUT_TICKS - 1)

typedef int (*timer_fn)(void *data);
struct timer {
	struct list_head entry;
	timer_fn fn;
	void *data;
	struct list_head wq;
};

/* Should we add barrier here? */
#define set_current_irq_ctx(v)						\
	do {								\
		current->in_irq_ctx = v;				\
	} while (0)

void		sched_timer_tick_soft();
void		sched_switch();
void		wake_up_preempt_disabled(struct list_head *wq);
#endif
