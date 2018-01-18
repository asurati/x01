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
#include <vm.h>
#include <io.h>

extern char vm_dev_start;
volatile void * const io_base = &vm_dev_start;


#ifdef QRPI2

#define IO_BASE_PA	0x3f000000
#define CTRL_BASE_PA	0x40000000

volatile void * const ctrl_base = &vm_dev_start + 1024*1024;

static void io_ctrl_init(struct mmu_map_req *r)
{
	int ret;

	r->va_start = (void *)ctrl_base;
	r->pa_start = CTRL_BASE_PA;

	ret = mmu_map(NULL, r);
	assert(ret == 0);
}

#else

#define IO_BASE_PA	0x20000000

#define io_ctrl_init(x)		do {} while (0)

#endif

void io_init()
{
	int ret;
	struct mmu_map_req r;

	r.va_start = (void *)io_base;
	r.pa_start = IO_BASE_PA;
	r.n = 1;
	r.mt = MT_DEV_SHR;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_SECTION;
	r.exec = 0;
	r.global = 1;
	r.shared = 1;
	r.domain = 0;

	ret = mmu_map(NULL, &r);
	assert(ret == 0);

	io_ctrl_init(&r);
}
