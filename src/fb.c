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
#include <mbox.h>
#include <vm.h>
#include <string.h>
#include <fb.h>

static struct fb_info fbi;

const struct fb_info *fb_info_get()
{
	return &fbi;
}

void fb_init()
{
	int ret;
	uintptr_t pa;
	struct mmu_map_req r;
	size_t sz;

	ret = vm_alloc(VMA_SLUB, VM_UNIT_4MB, 1, &fbi.addr);
	assert(ret == 0);

	fbi.width = 1024;
	fbi.height = 768;
	fbi.depth = 24;

	ret = mbox_fb_alloc(fbi.width, fbi.height, fbi.depth, &pa, &fbi.sz,
			    &fbi.pitch);
	assert(ret == 0);

	/* Keep the size SECTION-aligned for ease in mapping. */
	sz = ALIGN_UP(fbi.sz, 1024 * 1024);
	sz >>= SECTION_SIZE_SZ;

	/* Restrict the size to max 4MB. */
	assert(sz < 4);

	r.va_start = fbi.addr;
	r.pa_start = pa;
	r.n = sz;
	r.mt = MT_NRM_WTNA;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_SECTION;
	r.exec = 0;
	r.global = 1;
	r.shared = 0;
	r.domain = 0;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	memset(fbi.addr, 0, fbi.sz);
}
