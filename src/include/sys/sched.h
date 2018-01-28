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

#define THRD_QUOTA			5

#define	SCHED_RET_SWITCH		1
#define	SCHED_RET_RUN			2

struct context {
	uintptr_t is_fresh;
	uintptr_t cpsr;
	uintptr_t reg[13];
	uintptr_t lr;
};

/* Should we add barrier here? */
#define set_current_irq_ctx(v)						\
	do {								\
		current->in_irq_ctx = v;				\
	} while (0)

void		sched_timer_tick();
void		sched_switch();
#endif
