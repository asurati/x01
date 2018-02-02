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
#include <work.h>

#include <sys/work.h>

_ctx_proc
static int wq_worker(void *p)
{
	struct list_head lwq;
	struct work_queue *wq;
	struct work *wk;
	struct list_head *e;

	wq = p;
	while (1) {
		wait_event(&wq->waitq, wq->cond == 1);

		/* Multiple wake_ups can run before the lock is acquired.
		 * The condition variable remains set.
		 */
		lock_sched_lock(&wq->workq_lock);
		wq->cond = 0;
		lwq = wq->workq;
		init_list_head(&wq->workq);
		lock_sched_unlock(&wq->workq_lock);

		list_for_each(e, &lwq) {
			wk = list_entry(e, struct work, entry);
			wk->fn(wk->p);
		}
	}
	return 0;
}

_ctx_proc
int wq_init(struct work_queue *wq)
{
	init_list_head(&wq->waitq);
	wq->worker = sched_thread_create(wq_worker, wq);
	assert(wq->worker);
	/* TODO Init lock. */
	return 0;
}

_ctx_proc
int wq_work_add(struct work_queue *wq, struct work *w)
{
	lock_sched_lock(&wq->workq_lock);
	wq->cond = 1;
	list_add_tail(&w->entry, &wq->workq);
	lock_sched_unlock(&wq->workq_lock);

	wake_up(&wq->waitq);
	return 0;
}

