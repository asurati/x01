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

#ifndef _SYS_WORK_H_
#define _SYS_WORK_H_

/* Internal work queue API. Used by the providers of a work queue. */

#include <list.h>
#include <lock.h>
#include <sched.h>

struct work_queue {
	struct list_head waitq;
	struct list_head workq;
	struct lock workq_lock;
	struct thread *worker;
	int cond;
};

int	wq_init(struct work_queue *wq);
#endif
