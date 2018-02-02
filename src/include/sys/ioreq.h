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

#ifndef _SYS_IOREQ_H_
#define _SYS_IOREQ_H_

/* Internal IOREQ API. Used by the providers of an IO request queue. */
#include <list.h>
#include <lock.h>

typedef void (*ioq_fn)(struct io_req *ior);

struct io_req_queue {
	struct list_head in;
	int busy;
	struct lock in_lock;

	ioq_fn ioctl;
	ioq_fn rw;
	struct work_queue *ioc_wq;
};

void	ioq_init(struct io_req_queue *ioq, ioq_fn ioctl, ioq_fn rw);
void	ioq_sched_io_done(struct io_req_queue *ioq);
#endif
