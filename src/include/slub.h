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

#ifndef _SLUB_H_
#define _SLUB_H_

#include <types.h>

/* Slub supports allocations of sizes
 * 8,16,32,64,128,256,512,1K,2K,4K,
 * 8K,16K,32K,64K,128K,256K,512K,1M,2M,4M,
 * 8M,16M,32M,64M
 */

void	*kmalloc(size_t sz);
void	kfree(void *p);
#endif
