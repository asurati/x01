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
#include <io.h>
#include <timer.h>

#ifdef QRPI2
void timer_init()
{
	uint32_t cntdn;
	volatile uint32_t *ctl;

	/* Connect the cntpnsirq timer to the FIQ. */
	ctl = (volatile uint32_t *)(ctrl_base + 0x40);
	*ctl |= 1 << 5;

	cntdn = 62500000;

	/* cntp_tval. */
	asm volatile("mcr	p15, 0, %0, c14, c2, 0"
		     : : "r" (cntdn));

	/* cntp_ctl. */
	asm volatile("mrc	p15, 0, %0, c14, c2, 1"
		     : "=r" (cntdn));
	cntdn |= 1;
	asm volatile("mcr	p15, 0, %0, c14, c2, 1"
		     : : "r" (cntdn));

}
#else
#define timer_init	do {} while (0)
#endif
