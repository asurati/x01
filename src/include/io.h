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

/* 1MB device memory mapped at io_base. */
extern volatile void * const io_base;

#ifdef QRPI2

#define IO_BASE_PA	0x3f000000
#define CTRL_BASE_PA	0x40000000
extern volatile void * const ctrl_base;

#else /* QRPI2 */

#define IO_BASE_PA	0x20000000

#endif /* QRPI2 */

static inline uint32_t readl(const volatile void *addr)
{
	return *(const volatile uint32_t *)addr;
}

static inline void writel(uint32_t val, volatile void *addr)
{
	*(volatile uint32_t *)addr = val;

}

void	io_init();
#endif
