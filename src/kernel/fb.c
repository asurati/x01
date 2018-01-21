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
#include <ioreq.h>
#include <list.h>
#include <sched.h>
#include <slub.h>

static struct mbox_fb_buf *fbi;
static void *fb;

int fb_info_get(struct fb_info *fi)
{
	fi->width = fbi->phy_width;
	fi->height = fbi->phy_height;
	fi->depth = fbi->depth;
	fi->pitch = fbi->pitch;
	fi->addr = fb;
	fi->sz = fbi->sz;
	return 0;
}

void fb_init()
{
	int ret;
	struct list_head wq;
	struct io_req ior;
	struct mmu_map_req r;
	size_t sz;

	ret = vm_alloc(VMA_SLUB, VM_UNIT_4MB, 1, &fb);
	assert(ret == 0);

	fbi = kmalloc(sizeof(*fbi));
	assert(fbi);
	memset(fbi, 0, sizeof(*fbi));

	fbi->phy_width = fbi->virt_width = 1024;
	fbi->phy_height = fbi->virt_height = 768;
	fbi->depth = 24;

	init_list_head(&wq);
	memset(&ior, 0, sizeof(ior));
	ior.wq = &wq;
	ior.req = fbi;
	ior.sz = sizeof(*fbi);
	IOCTL(&ior, MBOX_IOCTL_FB_ALLOC, IOCTL_DIR_NONE);
	ret = mbox_io(&ior);
	assert(ret == IO_RET_PENDING);
	wait_event(&wq, BF_GET(ior.flags, IORF_DONE) == 1);

	/* Keep the size SECTION-aligned for ease in mapping. */
	sz = ALIGN_UP(fbi->sz, 1024 * 1024);
	sz >>= SECTION_SIZE_SZ;

	/* Restrict the size to max 4MB. */
	assert(sz < 4);

	r.va_start = fb;
	r.pa_start = fbi->addr;
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

	memset(fb, 0, fbi->sz);
}
