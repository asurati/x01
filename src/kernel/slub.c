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
#include <pm.h>
#include <slub.h>
#include <string.h>
#include <list.h>
#include <vm.h>
#include <mutex.h>

#define SLUB_NSIZES		24
#define SLUB_SUBPAGE_NSIZES	 9

#define SLUB_ALLOC_SIZES_SZ	 3
static const size_t slub_alloc_sizes[SLUB_NSIZES] = {
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024,
	2 * 1024,
	4 * 1024,
	8 * 1024,
	16 * 1024,
	32 * 1024,
	64 * 1024,
	128 * 1024,
	256 * 1024,
	512 * 1024,
	1024 * 1024,
	2 * 1024 * 1024,
	4 * 1024 * 1024,
	8 * 1024 * 1024,
	16 * 1024 * 1024,
	32 * 1024 * 1024,
	64 * 1024 * 1024,
};


/* The linked list used for SLUB_SUBPAGE_NSIZES slabs. */
struct slab {
	struct list_head entry;
	uint16_t nfree;
	uint16_t free;
	/* PAGE_SIZE aligned pointer. */
	void *p;
};

/* Coarse lock for all slabs. TODO: Implement finer locks. */
struct mutex slub_lock;

/* This variable is used to monitor the usage of the 16byte slabs.
 * If the number of free items are quite less, we need to use one
 * of them to allocate a struct slab to avoid deadlock of
 * needing a 16byte allocation to allocate a 16byte allocation.
 */
static int slub_16byte_free;

/* struct page va field. */
#define SLUB_LEADER_POS		 0
#define SLUB_LEADER_SZ		 1

static struct list_head subpage_full_heads[SLUB_SUBPAGE_NSIZES];
static struct list_head subpage_part_heads[SLUB_SUBPAGE_NSIZES];
static struct list_head subpage_none_heads[SLUB_SUBPAGE_NSIZES];

/* For each of the 9 sizes, we allocate one page as the data page
 * p[0]. The struct slab for each of the 9 sizes are allocated
 * from p[1], the 16 byte slab.
 */
void slub_init()
{
	int i, j, n, ret, shft;
	size_t sz;
	struct slab *s;
	struct page *pg;
	struct mmu_map_req r;
	uintptr_t pa[SLUB_SUBPAGE_NSIZES], t;
	void *va[SLUB_SUBPAGE_NSIZES];
	extern char vm_slub_start;

	mutex_init(&slub_lock);

	assert(sizeof(struct slab) == 16);

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		init_list_head(&subpage_full_heads[i]);
		init_list_head(&subpage_part_heads[i]);
		init_list_head(&subpage_none_heads[i]);
	}

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, SLUB_SUBPAGE_NSIZES, pa);
	assert(ret == 0);

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i)
		va[i] = &vm_slub_start + (i << PAGE_SIZE_SZ);

	r.n = 1;
	r.mt = MT_NRM_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		r.va_start = va[i];
		r.pa_start = pa[i];
		ret = mmu_map(&r);
		assert(ret == 0);
		memset(va[i], 0, PAGE_SIZE);
	}

	/* Set the free list on each page. */
	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		s = va[1];
		s += i;

		pg = pm_ram_get_page(pa[i]);
		assert(pg);
		assert(BF_GET(pg->flags, PGF_USE) == PGF_USE_SLUB);
		assert(BF_GET(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

		BF_SET(pg->flags, PGF_SLUB_SIZE, i);
		t = (uintptr_t)s;
		BF_SET(t, SLUB_LEADER, 1);
		pg->u0.va = (void *)t;

		s->p = va[i];
		shft = SLUB_ALLOC_SIZES_SZ + i;
		t = (uintptr_t)s->p;
		n = PAGE_SIZE >> shft;
		sz = slub_alloc_sizes[i];

		if (i == 1) {
			j = SLUB_SUBPAGE_NSIZES;
			s->nfree = slub_16byte_free = n - j;
			s->free = j;
			t += j << shft;
			list_add_tail(&s->entry, &subpage_part_heads[i]);
		} else {
			j = 0;
			s->free = 0;
			s->nfree = n;
			list_add_tail(&s->entry, &subpage_none_heads[i]);
		}

		for (j = j; j < n; ++j, t += sz)
			*(uint32_t *)t = j + 1;
	}
}

static struct slab *slub_alloc_slab(int ix)
{
	int i, n;
	void *p;
	struct slab *s;
	uintptr_t va;
	size_t sz;

	sz = slub_alloc_sizes[ix];
	n = PAGE_SIZE >> (SLUB_ALLOC_SIZES_SZ + ix);

	/* Allocate a struct slab and a data page. */
	s = kmalloc(sizeof(struct slab));
	assert(s);
	memset(s, 0, sizeof(struct slab));

	p = kmalloc(PAGE_SIZE);
	assert(p);
	memset(p, 0, PAGE_SIZE);

	s->p = p;
	s->nfree = n;
	va = (uintptr_t)p;
	for (i = 0; i < n; ++i, va += sz)
		*(uint32_t *)va = i + 1;
	return s;
}

void *kmalloc_page(int ix)
{
	int ret;
	void *p;
	uintptr_t pa;
	struct mmu_map_req r;

	/* For now, only allocate single page. We also
	 * need to track the allocations to support kfree().
	 */
	assert(ix == 9);

	ret = vm_alloc(VMA_SLUB, VM_UNIT_PAGE, 1, &p);
	assert(ret == 0);

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, 1, &pa);
	assert(ret == 0);

	r.va_start = p;
	r.pa_start = pa;
	r.n = 1;
	r.mt = MT_NRM_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	ret = mmu_map(&r);
	assert(ret == 0);

	return p;
}

void *kmalloc(size_t sz)
{
	int i, n, shft;
	void *p;
	uintptr_t va;
	struct slab *s;
	struct list_head *h, *e, *nh;

	if (sz == 0)
		return NULL;

	for (i = 0; i < SLUB_NSIZES; ++i)
		if (slub_alloc_sizes[i] >= sz)
			break;

	if (i >= SLUB_SUBPAGE_NSIZES)
		return kmalloc_page(i);

	/* subpage allocations. */

	mutex_lock(&slub_lock);

	shft = SLUB_ALLOC_SIZES_SZ + i;
	n = PAGE_SIZE >> shft;
	h = &subpage_part_heads[i];
	e = NULL;
	if (list_empty(h))
		h = &subpage_none_heads[i];

	if (list_empty(h)) {
		/* If we are unable to find any free item, the size
		 * which is now empty better not be the 16byte alloc
		 * size.
		 */
		assert(i != 1);
		s = slub_alloc_slab(i);
		list_add(&s->entry, h);
	}

	assert(!list_empty(h));
	e = h->next;

	s = list_entry(e, struct slab, entry);

	/* Get the current free. */
	va = (uintptr_t)s->p;

	/* Get the pointer to the corresponding object. */
	p = (void *)(va + (s->free << shft));
	s->free = *(uint32_t *)p;
	*(uint32_t *)p = 0;


	nh = NULL;
	if (s->nfree == n)
		nh = &subpage_part_heads[i];
	else if (s->nfree == 1)
		nh = &subpage_full_heads[i];

	--s->nfree;
	assert(s->nfree < n);

	if (nh) {
		list_del(e);
		list_add(e, nh);
	}

	if (i == 1)
		--slub_16byte_free;

	if (slub_16byte_free == 4) {
		s = slub_alloc_slab(1);
		list_add_tail(&s->entry, &subpage_none_heads[1]);
		slub_16byte_free += PAGE_SIZE >> (SLUB_ALLOC_SIZES_SZ + 1);
	}

	assert(p);
	mutex_unlock(&slub_lock);
	return p;
}

void kfree(void *p)
{
	int i, j, n, shft;
	uintptr_t pa, t;
	struct page *pg;
	struct slab *s;
	struct list_head *e, *nh;

	/* The struct page for the pointer p must have SLUB_LEADER set in va,
	 * and PGF_USE_SLUB set in flags.
	 */
	pa = mmu_va_to_pa(p);
	assert(pa != 0xffffffff);

	pg = pm_ram_get_page(pa);
	assert(BF_GET(pg->flags, PGF_USE) == PGF_USE_SLUB);

	t = (uintptr_t)pg->u0.va;
	assert(BF_GET(t, SLUB_LEADER));

	BF_CLR(t, SLUB_LEADER);
	s = (void *)t;

	i = BF_GET(pg->flags, PGF_SLUB_SIZE);

	assert(i < SLUB_SUBPAGE_NSIZES);

	/* subpage allocations occur in PAGE_SIZE pages. */
	assert(BF_GET(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

	shft = SLUB_ALLOC_SIZES_SZ + i;
	n = PAGE_SIZE >> shft;

	/* The pointer p must be appropriately aligned. */
	assert(((uintptr_t)p & (slub_alloc_sizes[i] - 1)) == 0);

	mutex_lock(&slub_lock);
	/* The slab->p must be a single page for subpage allocations. */
	assert(s->p <= p && p < s->p + PAGE_SIZE);

	j = p - s->p;
	j >>= shft;
	assert(j < n);

	/* Save the current free into the newly freed object,
	 * and set the newly freed object as s->free.
	 */
	*(uint32_t *)p = s->free;
	s->free = j;

	++s->nfree;
	assert(s->nfree <= n);
	nh = NULL;
	if (s->nfree == n)
		nh = &subpage_none_heads[i];	/* from part. */
	else if (s->nfree == 1)
		nh = &subpage_part_heads[i];	/* from full. */

	if (nh) {
		e = &s->entry;
		list_del(e);
		list_add(e, nh);
	}
	mutex_unlock(&slub_lock);
}
