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

#ifndef _IOREQ_H_
#define _IOREQ_H_

/* External IOREQ API. Used by the clients of a IO request queue. */

#include <types.h>
#include <mutex.h>
#include <work.h>

#define IOR_TYPE_READ					1
#define IOR_TYPE_WRITE					2
#define IOR_TYPE_IOCTL					3

struct io_req {
	struct list_head entry;
	char type;
	char async;
	char done;
	char pad;
	int status;
	union {
		struct {
			int cmd;
			void *arg;
		} ioctl;

		struct {
			void *data;
			size_t sz;
		} rw;
	} io;

	union {
		struct work ioc_work;
		struct list_head ioc_wait;
	} ioc;
};

struct io_req_queue;

void	ioq_ior_submit(struct io_req_queue *ioq, struct io_req *ior);
void	ioq_ior_wait(struct io_req *ior);
#endif
