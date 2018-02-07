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
void * const io_base = &vm_dev_start;

#ifdef QRPI2

#define IO_BASE_PA	0x3f000000
#define CTRL_BASE_PA	0x40000000

void * const ctrl_base = &vm_dev_start + 4 * 1024*1024;

static void io_ctrl_init(struct mmu_map_req *r)
{
	int ret;

	r->n = 1;
	r->va_start = (void *)ctrl_base;
	r->pa_start = CTRL_BASE_PA;

	ret = mmu_map(r);
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
	r.n = 4;		/* For UART, etc. */
	r.mt = MT_DEV_SHR;
	r.ap = AP_SRW;
	r.mu = MAP_UNIT_SECTION;
	r.flags  = bits_on(MMR_XN);
	r.flags |= bits_on(MMR_SHR);
	r.flags |= bits_on(MMR_AB);	/* Prevent access faults. */

	ret = mmu_map(&r);
	assert(ret == 0);

	io_ctrl_init(&r);
}
