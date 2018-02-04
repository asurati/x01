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
#define SLUB_FULLPAGE_NSIZES	(SLUB_NSIZES - SLUB_SUBPAGE_NSIZES)

#define SLUB_SUBPAGE_START	 3

/*
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
*/

/* Handles subpage allocations. */
struct subpage_slab {
	struct list_head entry;
	uint16_t nfree;
	uint16_t free;
	/* PAGE_SIZE aligned pointer. */
	void *p;
};

/* Handles full page allocations. */
struct fullpage_slab {
	struct list_head entry;
	void *p;
};

struct subpage {
	struct list_head busy;
	struct list_head part;
	struct list_head free;
	struct mutex lock;
	int log_sz;
};

/* full page allocations are all consider busy. */
struct fullpage {
	struct list_head busy;
	struct mutex lock;
	int log_sz;
};


/* This variable is used to monitor the usage of the 16byte slabs.
 * If the number of free items are quite less, we need to use one
 * of them to allocate a struct slab to avoid deadlock of
 * needing a 16byte allocation to allocate a 16byte allocation.
 */
static int slub_16byte_free;

/* struct page va field. */
#define SLUB_LEADER_POS		 0
#define SLUB_LEADER_SZ		 1

/* To support allocation of PTs, which are subpage-sized, but
 * still need to be kept apart from the normal user allocations
 * to avoid accidental overwrites or cache aliasing problems.
 */

static struct subpage_slab mmu_pt_slab;
static struct subpage mmu_pt_subpages;
static struct subpage subpages[SLUB_SUBPAGE_NSIZES];
static struct fullpage fullpages[SLUB_FULLPAGE_NSIZES];

static void slub_subpage_init0(struct subpage *sp, int log_sz)
{
	sp->log_sz = log_sz;
	init_list_head(&sp->busy);
	init_list_head(&sp->part);
	init_list_head(&sp->free);
	mutex_init(&sp->lock);
}

static void slub_map(void *va, uintptr_t pa)
{
	int ret;
	struct mmu_map_req r;

	r.n = 1;
	r.mt = MT_NRM_IO_WBA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_PAGE;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;
	r.va_start = va;
	r.pa_start = pa;

	ret = mmu_map(&r);
	assert(ret == 0);
	memset(va, 0, PAGE_SIZE);
}

static void slub_subpage_init1(struct subpage *sp, struct subpage_slab *sl,
			       void *va, uintptr_t pa)
{
	int i, n;
	uintptr_t t;
	size_t sz;
	struct page *pg;

	pg = pm_ram_get_page(pa);

	assert(pg);
	assert(bits_get(pg->flags, PGF_USE) == PGF_USE_SLUB);
	assert(bits_get(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

	/* Save the log(sz of the allocation unit). */
	pg->flags |= bits_set(PGF_SLUB_LSIZE, sp->log_sz);
	pg->u0.va  = (void *)((uintptr_t)sl | bits_on(SLUB_LEADER));

	sl->p = va;
	t = (uintptr_t)sl->p;
	n = PAGE_SIZE >> sp->log_sz;
	sz = 1 << sp->log_sz;

	/* subpages[1] is the slab responsible for 16byte allocations,
	 * which also happens to be the size of struct slab.
	 */
	if (sp->log_sz == 4) {
		/* We use up 9 struct slab elements straight away. */
		i = SLUB_SUBPAGE_NSIZES;
		sl->nfree = n - i;
		slub_16byte_free += sl->nfree;
		sl->free = i;
		t += i << sp->log_sz;
		list_add_tail(&sl->entry, &sp->part);
	} else {
		i = 0;
		sl->free = 0;
		sl->nfree = n;
		list_add_tail(&sl->entry, &sp->free);
	}

	/* Initialize the free list. */
	for (i = i; i < n; ++i, t += sz)
		*(uint32_t *)t = i + 1;
}

/* For each of the 9 sizes, we allocate one page as the data page
 * p[0]. The struct slab for each of the 9 sizes are allocated
 * from p[1], the 16 byte slab.
 */
void slub_init()
{
	int i, ret;
	struct subpage_slab *sl;
	uintptr_t pa[SLUB_SUBPAGE_NSIZES];
	void *va[SLUB_SUBPAGE_NSIZES];
	extern char vm_slub_end;
	extern char mmu_slub_area;

	assert(sizeof(struct subpage_slab) == 16);

	for (i = 0; i < SLUB_FULLPAGE_NSIZES; ++i) {
		fullpages[i].log_sz = PAGE_SIZE_SZ + i;
		init_list_head(&fullpages[i].busy);
		mutex_init(&fullpages[i].lock);
	}

	/* We need 0x400-byte allocations for the mmu to allocate PTs.
	 * The allocations come from a separate subpage instance. That
	 * subpage instance must be initialized first.
	 * For that, we get a slab page and map it to a va for which
	 * the PTE resides in the kernel PT k_pt.
	 */
	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, 1, pa);
	assert(ret == 0);
	va[0] = &mmu_slub_area;

	/* The mmu_map must succeed without it needing to call back
	 * into slub() for allocating a PT.
	 *
	 * (1 << 10) == 0x400.
	 */
	slub_subpage_init0(&mmu_pt_subpages, SLUB_SUBPAGE_START + 7);
	slub_map(va[0], pa[0]);
	slub_subpage_init1(&mmu_pt_subpages, &mmu_pt_slab, va[0], pa[0]);


	/* Any mmu_map() calls which require allocation of PTs can now
	 * work.
	 */

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, SLUB_SUBPAGE_NSIZES, pa);
	assert(ret == 0);

	va[0] = &vm_slub_end - (SLUB_SUBPAGE_NSIZES << PAGE_SIZE_SZ);

	/* Map the initial slub pages at the end of the vm_slub area. */
	for (i = 1; i < SLUB_SUBPAGE_NSIZES; ++i)
		va[i] = va[i - 1] + PAGE_SIZE;

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		slub_subpage_init0(&subpages[i], SLUB_SUBPAGE_START + i);
		slub_map(va[i], pa[i]);
	}

	for (i = 0; i < SLUB_SUBPAGE_NSIZES; ++i) {
		sl = va[1];
		sl += i;
		slub_subpage_init1(&subpages[i], sl, va[i], pa[i]);
	}
}

/* Must be at process context. */
static void slub_alloc_subpage_slab(struct subpage *sp)
{
	void *p;
	struct subpage_slab *sl;
	uintptr_t pa;

	/* Allocate a struct slab and a data page. */
	sl = kmalloc(sizeof(*sl));
	assert(sl);
	memset(sl, 0, sizeof(*sl));

	p = kmalloc(PAGE_SIZE);
	assert(p);
	memset(p, 0, PAGE_SIZE);

	pa = mmu_va_to_pa(p);
	assert(pa != 0xffffffff);

	slub_subpage_init1(sp, sl, p, pa);
}

static void *kmalloc_fullpages(struct fullpage *fp)
{
	int ret;
	void *p;
	struct page *pg;
	uintptr_t pa;
	struct fullpage_slab *sl;

	/* For now, only allocate single page. We also
	 * need to track the allocations to support kfree().
	 */
	assert(fp->log_sz == PAGE_SIZE_SZ);

	ret = vm_alloc(VMA_SLUB, VM_UNIT_PAGE, 1, &p);
	assert(ret == 0);

	ret = pm_ram_alloc(PM_UNIT_PAGE, PGF_USE_SLUB, 1, &pa);
	assert(ret == 0);

	slub_map(p, pa);

	sl = kmalloc(sizeof(*sl));
	assert(sl);
	sl->p = p;

	pg = pm_ram_get_page(pa);
	assert(pg);

	assert(bits_get(pg->flags, PGF_USE) == PGF_USE_SLUB);

	/* The UNIT value depends on the index of allocation. */
	assert(bits_get(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

	/* For multiplage allocations, only the first page has
	 * the LEADER bit set.
	 */
	pg->flags |= bits_set(PGF_SLUB_LSIZE, fp->log_sz);
	pg->u0.va  = (void *)((uintptr_t)sl | bits_on(SLUB_LEADER));

	mutex_lock(&fp->lock);
	list_add_tail(&sl->entry, &fp->busy);
	mutex_unlock(&fp->lock);

	return p;
}

static void *kmalloc_subpages(struct subpage *sp)
{
	int n;
	void *p;
	uintptr_t va;
	struct subpage_slab *sl;
	struct list_head *h, *e, *nh;

	mutex_lock(&sp->lock);

	n = PAGE_SIZE >> sp->log_sz;
	h = &sp->part;
	e = NULL;
	if (list_empty(h))
		h = &sp->free;

	if (list_empty(h)) {
		/* If we are unable to find any free item, the size
		 * which is now empty better not be the 16byte alloc
		 * size.
		 */
		assert(sp->log_sz != 4);
		slub_alloc_subpage_slab(sp);
	}

	assert(!list_empty(h));
	e = h->next;

	sl = list_entry(e, struct subpage_slab, entry);

	/* Get the current free. */
	va = (uintptr_t)sl->p;

	/* Get the pointer to the corresponding object. */
	p = (void *)(va + (sl->free << sp->log_sz));
	sl->free = *(uint32_t *)p;
	*(uint32_t *)p = 0;

	nh = NULL;
	if (sl->nfree == n)
		nh = &sp->part;
	else if (sl->nfree == 1)
		nh = &sp->busy;

	--sl->nfree;
	assert(sl->nfree < n);

	if (nh) {
		list_del(e);
		list_add(e, nh);
	}

	if (sp->log_sz == 4) {
		--slub_16byte_free;
		if (slub_16byte_free == 4)
			slub_alloc_subpage_slab(sp);
	}
	mutex_unlock(&sp->lock);

	assert(p);
	return p;
}

void *kmalloc(size_t sz)
{
	int i;

	if (sz == 0)
		return NULL;

	for (i = 0; i < SLUB_NSIZES; ++i)
		if ((1u << (SLUB_SUBPAGE_START + i)) >= sz)
			break;

	assert (i < SLUB_NSIZES);

	if (i >= SLUB_SUBPAGE_NSIZES)
		return kmalloc_fullpages(&fullpages[i - SLUB_SUBPAGE_NSIZES]);
	else
		return kmalloc_subpages(&subpages[i]);
}

void kfree_fullpages(void *p, struct fullpage *fp, struct page *pg,
		     uintptr_t pa, struct fullpage_slab *sl)
{
	int ret;
	(void)fp;
	(void)pg;

	list_del(&sl->entry);
	kfree(sl);

	ret = pm_ram_free(PM_UNIT_PAGE, PGF_USE_SLUB, 1, &pa);
	assert(ret == 0);

	ret = vm_free(VMA_SLUB, VM_UNIT_PAGE, 1, (const void **)&p);
	assert(ret == 0);
}

void kfree_subpages(void *p, struct subpage *sp, struct page *pg, uintptr_t pa,
		     struct subpage_slab *sl)
{
	int j, n;
	struct list_head *nh;

	(void)pa;

	/* subpage allocations occur in PAGE_SIZE pages. */
	assert(bits_get(pg->flags, PGF_UNIT) == PM_UNIT_PAGE);

	n = PAGE_SIZE >> sp->log_sz;

	/* The pointer p must be appropriately aligned. */
	assert(((uintptr_t)p & ((1 << sp->log_sz) - 1)) == 0);

	sp = &subpages[sp->log_sz - SLUB_SUBPAGE_START];
	mutex_lock(&sp->lock);
	/* The slab->p must be a single page for subpage allocations. */
	assert(sl->p <= p && p < sl->p + PAGE_SIZE);

	j = p - sl->p;
	j >>= sp->log_sz;
	assert(j < n);

	/* Save the current free into the newly freed object,
	 * and set the newly freed object as s->free.
	 */
	*(uint32_t *)p = sl->free;
	sl->free = j;

	++sl->nfree;
	assert(sl->nfree <= n);
	nh = NULL;

	if (sl->nfree == n)
		nh = &sp->free;	/* from part. */
	else if (sl->nfree == 1)
		nh = &sp->part;	/* from full. */

	if (nh) {
		list_del(&sl->entry);
		list_add(&sl->entry, nh);
	}
	mutex_unlock(&sp->lock);
}

void kfree(void *p)
{
	int i;
	void *va;
	uintptr_t pa;
	struct page *pg;

	/* The struct page for the pointer p must have SLUB_LEADER set in va,
	 * and PGF_USE_SLUB set in flags.
	 */
	pa = mmu_va_to_pa(p);
	assert(pa != 0xffffffff);

	pg = pm_ram_get_page(pa);
	assert(bits_get(pg->flags, PGF_USE) == PGF_USE_SLUB);

	va = pg->u0.va;
	assert(bits_get((uintptr_t)va, SLUB_LEADER));
	va = (void *)((uintptr_t)va & bits_off(SLUB_LEADER));

	i = bits_get(pg->flags, PGF_SLUB_LSIZE) - SLUB_SUBPAGE_START;

	if (i >= SLUB_SUBPAGE_NSIZES)
		kfree_fullpages(p, &fullpages[i - SLUB_SUBPAGE_NSIZES], pg, pa,
				(struct fullpage_slab *)va);
	else
		kfree_subpages(p, &subpages[i], pg, pa,
			       (struct subpage_slab *)va);
}

void *mmu_slub_alloc()
{
	return kmalloc_subpages(&mmu_pt_subpages);
}

void mmu_slub_free(void *p)
{
	int i;
	void *va;
	uintptr_t pa;
	struct page *pg;
	/* The struct page for the pointer p must have SLUB_LEADER set in va,
	 * and PGF_USE_SLUB set in flags.
	 */
	pa = mmu_va_to_pa(p);
	assert(pa != 0xffffffff);

	pg = pm_ram_get_page(pa);
	assert(bits_get(pg->flags, PGF_USE) == PGF_USE_SLUB);

	va = pg->u0.va;
	assert(bits_get((uintptr_t)va, SLUB_LEADER));
	va = (void *)((uintptr_t)va & bits_off(SLUB_LEADER));

	i = bits_get(pg->flags, PGF_SLUB_LSIZE);
	assert(i == 10);
	kfree_subpages(p, &mmu_pt_subpages, pg, pa, (struct subpage_slab *)va);
}

/* pa must be appropriately aligned.
 * Called with PD lock held.
 */

void *mmu_slub_pa_to_va(uintptr_t pa)
{
	int i;
	void *va;
	struct page *pg;
	const struct subpage_slab *sl;

	pg = pm_ram_get_page(pa);
	assert(bits_get(pg->flags, PGF_USE) == PGF_USE_SLUB);

	va = pg->u0.va;
	assert(bits_get((uintptr_t)va, SLUB_LEADER));
	va = (void *)((uintptr_t)va & bits_off(SLUB_LEADER));

	i = bits_get(pg->flags, PGF_SLUB_LSIZE);
	assert(i == 10);

	sl = (const struct subpage_slab *)va;

	return sl->p + (pa & PAGE_SIZE_MASK);
}

