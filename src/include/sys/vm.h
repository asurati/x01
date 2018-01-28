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

#ifndef _SYS_VM_H_
#define _SYS_VM_H_

#include <list.h>

#define VSF_AP_POS		 0
#define VSF_AP_SZ		 3
#define VSF_MT_POS		 3
#define VSF_MT_SZ		 5
#define VSF_NPAGES_POS		12
#define VSF_NPAGES_SZ		20

/* For now, the vm calls support allocation and deallocation within the
 * two 128MB region starting at vm_slub_start.
 */
#define VM_AREA_SIZE	(128 * 1024 * 1024)

struct vm_seg {
	struct list_head entry;
	void *start;
	uint32_t flags;
};
#endif
