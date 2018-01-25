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

#ifndef _BARRIER_H_
#define _BARRIER_H_

#define barrier()							\
	do {								\
		asm volatile("" : : : "memory");			\
	} while (0)

#define dmb()								\
	do {								\
		asm volatile("mcr	p15, 0, %0, c7, c10, 5"		\
			     : : "r" (0) : "memory");			\
	} while (0)

#define dsb() \
	do { \
		asm volatile("mcr	p15, 0, %0, c7, c10, 4\n\t" \
			     : : "r" (0)); \
	} while (0)

#endif
