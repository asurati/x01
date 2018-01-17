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

#define THRD_STATE_RUNNING		1
#define THRD_STATE_EVICTING		2
#define THRD_STATE_READY		3

#define THRD_QUOTA			5

struct thread {
	struct list_head entry;
	void *usr_stack_hi;
	void *svc_stack_hi;
	void *context;
	char quota;
	char ticks;
	char preemptible;
	char state;
};

typedef int (*thread_fn)(void *p);

void		sched_init();
void		sched_timer_tick();
struct thread	*sched_thread_create(thread_fn fn, void *p);
#endif
