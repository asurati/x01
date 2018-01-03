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

#ifndef _PM_H_
#define _PM_H_

#include <bdy.h>

/* PM_UNIT_MAX must equal BDY_NLEVELS. */
enum pm_alloc_units {
	PM_UNIT_4KB,
	PM_UNIT_8KB,
	PM_UNIT_16KB,
	PM_UNIT_32KB,
	PM_UNIT_64KB,
	PM_UNIT_128KB,
	PM_UNIT_256KB,
	PM_UNIT_512KB,
	PM_UNIT_1MB,
	PM_UNIT_2MB,
	PM_UNIT_4MB,
	PM_UNIT_8MB,
	PM_UNIT_16MB,
	PM_UNIT_MAX
};

#define PM_UNIT_PAGE		PM_UNIT_4KB
#define PM_UNIT_LARGE_PAGE	PM_UNIT_64KB
#define PM_UNIT_SECTION		PM_UNIT_1MB
#define PM_UNIT_SUPER_SECTION	PM_UNIT_16MB

void	pm_init(uint32_t ram, uint32_t ramsz);
int	pm_ram_alloc(enum pm_alloc_units unit, int n, uintptr_t *pa);
int	pm_ram_free(enum pm_alloc_units unit, int n, const uintptr_t *pa);

#endif
