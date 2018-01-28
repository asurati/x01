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

/* If the struct is for a page which is a subordinate page to
 * either a large page, a section or a super section, then all
 * fields (except the TYPE bit) on such a subordinate are free
 * and can be used by the leader page as an extension.
 *
 * Whether a page is a leader can be known by the physical address
 * and the type.
 *
 * The busy/free bit for the page is stored in the buddy bitmap.
 */

enum pm_page_usage {
	PGF_USE_NORMAL,
	PGF_USE_SLUB
};

/* PGF_UNIT has the same values as enum pm_alloc_units.
 * The UNIT allocation implies physically contiguous memory. Thus,
 * a successful allocation of a super section implies a physically
 * contiguous range of 16MB memory.
 */
#define PGF_UNIT_POS			0
#define PGF_UNIT_SZ			4
#define PGF_USE_POS			4
#define PGF_USE_SZ			1

/* Only valid if PGF_USE_SLUB is true. */
#define PGF_SLUB_LSIZE_POS		5
#define PGF_SLUB_LSIZE_SZ		5

struct page {
	uint32_t flags;
	union {
		int ref;
		void *va;		/* struct slab. */
	} u0;
};

int		pm_ram_alloc(enum pm_alloc_units unit, enum pm_page_usage use,
			     int n, uintptr_t *pa);
int		pm_ram_free(enum pm_alloc_units unit, enum pm_page_usage use,
			    int n, const uintptr_t *pa);
struct page	*pm_ram_get_page(uintptr_t pa);
#endif
