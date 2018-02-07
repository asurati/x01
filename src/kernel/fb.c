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

#include <mbox.h>
#include <vm.h>
#include <string.h>
#include <fb.h>
#include <ioreq.h>
#include <list.h>
#include <sched.h>
#include <slub.h>

static struct mbox_fb_buf b __attribute__ ((aligned(16)));
void *fb;
const struct mbox_fb_buf *fbi = &b;

void fb_init()
{
	int ret;
	struct mmu_map_req r;
	size_t sz;

	memset(&b, 0, sizeof(b));

	b.phy_width = b.virt_width = 1024;
	b.phy_height = b.virt_height = 768;
	b.depth = 24;

	ret = mbox_fb_alloc(&b);
	assert(ret == 0);

	/* Keep the size SECTION-aligned for ease in mapping. */
	sz = ALIGN_UP(b.sz, SECTION_SIZE);
	sz >>= SECTION_SIZE_SZ;

	/* Restrict the size to max 4MB. */
	assert(sz < 4);

	ret = vm_alloc(VMA_SLUB, VM_UNIT_4MB, 1, &fb);
	assert(ret == 0);

	r.va_start = fb;
	r.pa_start = b.addr;
	r.n = sz;
	r.mt = MT_NRM_IO_NC;		/* Write Combining on armv6. */
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_SECTION;
	r.flags  = bits_on(MMR_XN);
	r.flags |= bits_on(MMR_AB);	/* Prevent access faults. */

	ret = mmu_map(&r);
	assert(ret == 0);

	memset(fb, 0, b.sz);
}
