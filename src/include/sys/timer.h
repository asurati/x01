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

#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <types.h>

#define HZ			1

/* Internal API for timer implementation. */

char		timer_is_asserted();
void		timer_rearm(uint32_t freq);
void		timer_disable();
void		timer_enable();
uint32_t	timer_freq();
#endif
