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

#define SLUB_NSIZES		24
#define SLUB_SUBPAGE_NSIZES	 9

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

	/* PAGE_SIZE aligned pointers. */
	void *p[2];
};

/* This variable is used to monitor the usage of the 16byte slabs.
 * If the number of free items are quite less, we need to use one
 * of them to allocate a struct slab to avoid deadlock of
 * needing a 16byte allocation to allocate a 16byte allocation.
 */
static int slub_16byte_free;

/* struct page slub field. */

/* 24 sizes support. Hence, 5-bit field. */
#define SLUB_SIZE_POS		 0
#define SLUB_SIZE_SZ		 5

#define SLUB_LEADER_POS		 5
#define SLUB_LEADER_SZ		 1

/* struct slab p field. */

/* The slab pointer stored in the p field is PAGE_SIZE aligned.
 * Thus 12-bits are available for use.
 */

/* Whether the slab is completely utilized. */
#define SLUB_FULL_POS		 0
#define SLUB_FULL_SZ		 1

/* Index of the head of the free item list. */
#define SLUB_FREE_POS		 1
#define SLUB_FREE_SZ		 9

static struct list_head subpage_full_heads[SLUB_SUBPAGE_NSIZES];
static struct list_head subpage_part_heads[SLUB_SUBPAGE_NSIZES];
static struct list_head subpage_none_heads[SLUB_SUBPAGE_NSIZES];

void slub_init()
{
	int i, n, ret;
	extern char slub_16byte_start;
	uintptr_t pa, va;
	struct mmu_map_req r;
	struct page *pg;
	struct slab *s;

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		init_list_head(&subpage_full_heads[i]);
		init_list_head(&subpage_part_heads[i]);
		init_list_head(&subpage_none_heads[i]);
	}
	assert(sizeof(struct slab) == 16);

	s = (struct slab *)&slub_16byte_start;

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, 1, &pa);
	assert(ret == 0);

	r.va_start = s;
	r.pa_start = pa;
	r.n = 1;
	r.mt = MT_NRM_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	pg = pm_ram_get_page(pa);
	assert(pg);
	assert(BF_GET(pg->flags, PGF_USE) == PGF_USE_SLUB);
	assert(BF_GET(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

	BF_SET(pg->u0.slub, SLUB_SIZE, 1);
	BF_SET(pg->u0.slub, SLUB_LEADER, 1);
	memset(s, 0, PAGE_SIZE);

	/* The first object of this slab is utilized to allocate a
	 * struct slab object for the 16byte allocation size
	 */
	va = (uintptr_t)s;
	BF_SET(va, SLUB_FREE, 1);
	s->p[0] = (void *)va;

	n = PAGE_SIZE/sizeof(struct slab);
	va = (uintptr_t)(s + 1);
	for (i = 1; i < n; ++i, va += sizeof(struct slab))
		*(uint32_t *)va = i + 1;

	list_add_tail(&s->entry, &subpage_part_heads[1]);
	slub_16byte_free = n - 1;
}

struct slab *slub_scan(const struct list_head *h)
{
	int i;
	uintptr_t va;
	struct slab *s;
	struct list_head *e;

	list_for_each(e, h) {
		s = list_entry(e, struct slab, entry);
		for (i = 0; i < 2; ++i) {
			if (s->p[i] == NULL)
				continue;
			va = (uintptr_t)s->p[i];
			if (BF_GET(va, SLUB_FULL))
				continue;
			return s;
		}
	}
	return NULL;
}


void *kmalloc_page(int ix)
{
	ix = ix;
	return NULL;
}

void *kmalloc(size_t sz)
{
	int i, j, n, ix;
	void *p;
	uintptr_t va;
	struct slab *s;
	struct list_head *h;

	if (sz == 0)
		return NULL;

	for (i = 0; i < SLUB_NSIZES; ++i)
		if (slub_alloc_sizes[i] >= sz)
			break;

	assert(i < SLUB_NSIZES);

	if (i >= SLUB_SUBPAGE_NSIZES)
		return kmalloc_page(i);

	h = &subpage_part_heads[i];
	s = slub_scan(h);

	if (s == NULL) {
		h = &subpage_none_heads[i];
		s = slub_scan(h);
	}

	if (s == NULL) {
		/* If we are unable to find any free item, the size
		 * which is now empty better not be the 16byte alloc
		 * size.
		 */
		assert(i != 1);

		/* Allocate a struct slab and a data page. */
		s = kmalloc(sizeof(struct slab));
		assert(s);
		memset(s, 0, sizeof(struct slab));

		p = kmalloc(PAGE_SIZE);
		assert(p);
		memset(p, 0, PAGE_SIZE);

		s->p[0] = p;
		va = (uintptr_t)p;

		n = slub_alloc_sizes[i];
		va = (uintptr_t)p;
		for (i = 0; i < n; ++i, va += n)
			*(uint32_t *)va = i + 1;

		list_add_tail(&s->entry, h);
		s = slub_scan(h);
		assert(s);
	}

	p = NULL;

	ix = i;

	for (i = 0; i < 2; ++i) {
		if (s->p[i] == NULL)
			continue;
		va = (uintptr_t)s->p[i];
		if (BF_GET(va, SLUB_FULL))
			continue;
		j = BF_GET(va, SLUB_FREE);
		BF_CLR(va, SLUB_FREE);

		p = (void *)(va + j * slub_alloc_sizes[ix]);
		j = *(uint32_t *)p;
		*(uint32_t *)p = 0;

		BF_SET(va, SLUB_FREE, j);
		s->p[i] = (void *)va;
		break;
	}

	if (ix == 1)
		--slub_16byte_free;

	assert(p);
	return p;
}


