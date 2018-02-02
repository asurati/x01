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

#ifndef _WORK_H_
#define _WORK_H_

#include <list.h>
#include <sched.h>

/* Work Queues to run deferred operations at _ctx_proc. */

/* External work queu API. Used by the clients of a work queue. */
struct work_queue;

typedef int (*work_fn)(void *data);

struct work {
	struct list_head entry;
	work_fn fn;
	void *p;
};

int	wq_work_add(struct work_queue *wq, struct work *w);
#endif
