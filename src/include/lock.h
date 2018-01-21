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

#ifndef _LOCK_H_
#define _LOCK_H_

#include <irq.h>
#include <sched.h>

/* LOCKs allow sychronization with IRQs, soft IRQs and
 * scheduler (preemption). Preemption is disabled while the lock is held.
 */

struct lock {
	int value;
};

#define lock_irq_lock(x)						\
	do {(void)x; irq_hard_disable(); } while (0);
#define lock_irq_unlock(x)						\
	do {(void)x; irq_hard_enable(); } while (0);

#define lock_irq_soft_lock(x)						\
	do {(void)x; irq_soft_disable(); } while (0);
#define lock_irq_soft_unlock(x)						\
	do {(void)x; irq_soft_enable(); } while (0);

#define lock_sched_lock(x)						\
	do {(void)x; preempt_disable(); } while (0);
#define lock_sched_unlock(x)						\
	do {(void)x; preempt_enable(); } while (0);

#endif
