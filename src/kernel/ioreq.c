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

#include <assert.h>
#include <ioreq.h>
#include <slub.h>

#include <sys/sched.h>
#include <sys/ioreq.h>
#include <sys/work.h>

static struct work_queue ioc_wq;

_ctx_proc
void ioq_init(struct io_req_queue *ioq, ioq_fn ioctl, ioq_fn rw)
{
	init_list_head(&ioq->in);
	ioq->busy = 0;
	ioq->ioctl = ioctl;
	ioq->rw = rw;

	/* Single queue for processing completions of IO belonging to
	 * all devices.
	 */
	ioq->ioc_wq = &ioc_wq;
}

_ctx_sched
static struct io_req *ioq_ior_next(struct io_req_queue *ioq)
{
	struct list_head *e;

	e = NULL;
	if (ioq->busy == 0 && !list_empty(&ioq->in)) {
		++ioq->busy;
		/* Do not unlink the request yet. */
		e = ioq->in.next;
	}
	if (e == NULL)
		return NULL;
	else
		return list_entry(e, struct io_req, entry);
}

/* The routine can run at both levels, but an instance running at _ctx_proc
 * is safe from interference by that running at _ctx_sched because the
 * busy flag ensures that at most one instance is running at any time.
 *
 * Alternatively, the ioq_sched_io_done should always queue the completion
 * to the work queue, where the routine will start the next IO. But such
 * arrangement requires ioreq specific routine in the work, which must
 * then call the client's routine.
 *
 */
_ctx_proc
_ctx_sched
static void ioq_ior_run(struct io_req_queue *ioq, struct io_req *ior)
{
	if (ior == NULL)
		return;

	switch (ior->type) {
	case IOR_TYPE_IOCTL:
		ioq->ioctl(ior);
		break;
	default:
		assert(0);
		break;
	}
}

_ctx_sched
struct io_req *ioq_ior_dequeue_sched(struct io_req_queue *ioq)
{
	struct list_head *e;

	--ioq->busy;
	e = list_del_head(&ioq->in);
	return list_entry(e, struct io_req, entry);
}

_ctx_sched
void ioq_ior_done_sched(struct io_req_queue *ioq, struct io_req *ior)
{
	ior->done = 1;
	if (ior->async)
		wq_work_add(ioq->ioc_wq, &ior->ioc.ioc_work);
	else
		wake_up_preempt_disabled(&ior->ioc.ioc_wait);

	ior = ioq_ior_next(ioq);
	ioq_ior_run(ioq, ior);
}

_ctx_proc
void ioq_ior_submit(struct io_req_queue *ioq, struct io_req *ior)
{
	ior->done = 0;
	if (ior->async == 0)
		init_list_head(&ior->ioc.ioc_wait);

	lock_sched_lock(&ioq->in_lock);
	list_add_tail(&ior->entry, &ioq->in);
	ior = ioq_ior_next(ioq);
	lock_sched_unlock(&ioq->in_lock);

	ioq_ior_run(ioq, ior);
}

/* Only for synchronous wait for the completion. */
_ctx_proc
void ioq_ior_wait(struct io_req *ior)
{
	assert(ior->async == 0);
	wait_event(&ior->ioc.ioc_wait, ior->done == 1);
}

_ctx_init
void ioreq_init()
{
	wq_init(&ioc_wq);
}
