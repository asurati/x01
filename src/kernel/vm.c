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

#include <assert.h>
#include <mmu.h>
#include <slub.h>
#include <string.h>
#include <vm.h>
#include <mutex.h>

#include <sys/vm.h>

extern char vm_slub_start;

/* Same order as enum vm_area. */
static void *vm_area_start[] = {
	&vm_slub_start,
};

static struct list_head vm_areas[VMA_MAX];
static struct mutex vm_areas_lock[VMA_MAX];

void vm_init()
{
	int i;
	struct vm_seg *slabs;
	extern char vm_slub_end;

	for (i = 0; i < VMA_MAX; ++i) {
		init_list_head(&vm_areas[i]);
		mutex_init(&vm_areas_lock[i]);
	}

	/* The last 9 pages of vm_slub area are utilized as slabs. */
	slabs = kmalloc(sizeof(*slabs));
	memset(slabs, 0, sizeof(*slabs));

	slabs->start = &vm_slub_end - (9 << PAGE_SIZE_SZ);;
	slabs->flags |= bits_set(VSF_NPAGES, 9);

	list_add(&slabs->entry, &vm_areas[VMA_SLUB]);
}

static const struct vm_seg *vm_find_seg(struct list_head *area,
					const void *start,
					const void *end)
{
	void *send;
	struct list_head *e;
	struct vm_seg *seg;

	list_for_each_rev(e, area) {
		seg = list_entry(e, struct vm_seg, entry);
		send = seg->start + bits_pull(seg->flags, VSF_NPAGES);
		if (seg->start <= start && start < send)
			return seg;
		if (seg->start < end && end <= send)
			return seg;
		if (start <= seg->start && seg->start < end)
			return seg;
		if (start < send && send <= end)
			return seg;
	}

	return NULL;
}

static void *vm_find_free(enum vm_area area,
			  enum vm_alloc_units unit)
{
	void *as, *ae, *t;
	size_t align;

	as = vm_area_start[area];
	ae = as + VM_AREA_SIZE;
	align = PAGE_SIZE << unit;
	t = ae - align;

	for (t = t; t >= as; t -= align) {
		if (vm_find_seg(&vm_areas[area], t, t + align))
			continue;
		return t;
	}
	return NULL;
}

int vm_alloc(enum vm_area area, enum vm_alloc_units unit, int n,
	     void **va)
{
	int i;
	struct vm_seg *seg;

	assert(area < VMA_MAX);

	/* Search for naturally aligned and sized block of free virtual
	 * memory
	 */

	mutex_lock(&vm_areas_lock[area]);
	for (i = 0; i < n; ++i) {
		va[i] = vm_find_free(area, unit);
		assert(va[i]);
		seg = kmalloc(sizeof(*seg));
		seg->start = va[i];
		seg->flags = bits_set(VSF_NPAGES, 1u << unit);
		list_add(&seg->entry, &vm_areas[area]);
	}
	mutex_unlock(&vm_areas_lock[area]);

	/* TODO free all and return error if at least one allocation
	 * failed
	 */
	return 0;
}

int vm_free(enum vm_area area, enum vm_alloc_units unit, int n,
	    const void **va)
{
	int i;
	struct list_head *e;
	struct vm_seg *seg;

	assert(area < VMA_MAX);
	mutex_lock(&vm_areas_lock[area]);
	list_for_each(e, &vm_areas[area]) {
		seg = list_entry(e, struct vm_seg, entry);
		for (i = 0; i < n; ++i) {
			if (va[i] == NULL)
				continue;
			if (seg->start != va[i])
				continue;
			assert(bits_get(seg->flags, VSF_NPAGES) ==
			       (1u << unit));
			list_del(&seg->entry);
			kfree(seg);
			va[i] = NULL;
		}
	}
	mutex_unlock(&vm_areas_lock[area]);
	return 0;
}
