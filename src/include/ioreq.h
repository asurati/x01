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

#include <types.h>

#define IO_RET_PENDING			 0x33

#define IORF_DONE_POS			 0
#define IORF_DONE_SZ			 1

#define IORF_TYPE_POS			 1
#define IORF_TYPE_SZ			 4

#define IORF_TYPE_READ			1
#define IORF_TYPE_WRITE			2
#define IORF_TYPE_IOCTL			3

#define IOCTL_CODE_POS			 0
#define IOCTL_CODE_SZ			16
#define IOCTL_DIR_POS			16
#define IOCTL_DIR_SZ			 2

/* Read and Write from the perspective of the IO requestor. */
#define IOCTL_DIR_NONE			 0
#define IOCTL_DIR_READ			 1
#define IOCTL_DIR_WRITE			 2

#define IOCTL(r, cd, dir)						\
	do {								\
		BF_SET((r)->flags, IORF_TYPE, IORF_TYPE_IOCTL);		\
		BF_SET((r)->u.ioctl.code, IOCTL_DIR, dir);		\
		BF_SET((r)->u.ioctl.code, IOCTL_CODE, cd);		\
	} while (0)

struct io_req_ioctl {
	int code;
};

struct io_req {
	struct list_head *wq;
	struct list_head entry;
	int flags;
	int ret;
	void *drv_data;	/* For driver's use. */
	const void *req;
	size_t sz;
	union {
		struct io_req_ioctl ioctl;
	} u;
};

#endif
