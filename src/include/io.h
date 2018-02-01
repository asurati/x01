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

#ifndef _IO_H_
#define _IO_H_

#include <types.h>

/* 1MB device memory mapped at io_base. */
extern void * const io_base;

#ifdef QRPI2
extern void * const ctrl_base;
#endif /* QRPI2 */

/* Word here stands for 16bits. */
static inline uint16_t readw(const void *addr)
{
	return *(const volatile uint16_t *)addr;
}

static inline uint32_t readl(const void *addr)
{
	return *(const volatile uint32_t *)addr;
}

static inline void writel(uint32_t val, void *addr)
{
	*(volatile uint32_t *)addr = val;

}
#endif
