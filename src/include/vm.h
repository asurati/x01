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

#ifndef _VM_H_
#define _VM_H_

#include <list.h>
#include <mmu.h>

enum vm_area {
	VMA_SLUB,
	VMA_MAX
};

enum vm_alloc_units {
	VM_UNIT_4KB,
	VM_UNIT_8KB,
	VM_UNIT_16KB,
	VM_UNIT_32KB,
	VM_UNIT_64KB,
	VM_UNIT_128KB,
	VM_UNIT_256KB,
	VM_UNIT_512KB,
	VM_UNIT_1MB,
	VM_UNIT_2MB,
	VM_UNIT_4MB,
	VM_UNIT_8MB,
	VM_UNIT_16MB,
	VM_UNIT_MAX
};

#define VM_UNIT_PAGE		VM_UNIT_4KB
#define VM_UNIT_LARGE_PAGE	VM_UNIT_64KB
#define VM_UNIT_SECTION		VM_UNIT_1MB
#define VM_UNIT_SUPER_SECTION	VM_UNIT_16MB

#define VSF_AP_POS		 0
#define VSF_AP_SZ		 3
#define VSF_MT_POS		 3
#define VSF_MT_SZ		 5
#define VSF_NPAGES_POS		12
#define VSF_NPAGES_SZ		20

struct vm_seg {
	struct list_head entry;
	void *start;
	uint32_t flags;
};

void		vm_init();
int		vm_alloc(enum vm_area area, enum vm_alloc_units unit, int n,
			 void **va);
int		vm_free(enum vm_area area, enum vm_alloc_units unit, int n,
			const void **va);
#endif
