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

OUTPUT_FORMAT(elf32-littlearm)
ENTRY(start)

/* Corresponds to the TTBCR.N setting. */
KMODE_VA = 0x40000000;
KRNL_SZ = 0x400000;

SECTIONS
{
	. = LOAD_PA;
	.boot : {
		boot/start.o(.text);
		boot/boot.o(.text);
		boot/boot.o(.bss);
		boot/boot.o(.rodata);
	}

	. = ALIGN(0x1000) + KMODE_VA;
	text_start = .;
	.text : AT (text_start - KMODE_VA) {
		*(.text);
		text_end = .;
	}

	. = ALIGN(0x1000);
	rodata_start = .;
	.rodata : AT (rodata_start - KMODE_VA) {
		*(.rodata);
		rodata_end = .;
	}

	. = ALIGN(0x1000);
	data_start = .;
	.data : AT (data_start - KMODE_VA) {
		*(.data);
		data_end = .;
	}

	. = ALIGN(0x1000);
	bss_start = .;
	.bss : AT (bss_start - KMODE_VA) {
		. += 0x1000;
		stack_hi = .;
		stack_hi_pa = . - KMODE_VA;

		/* Buddy map for RAM. 2 pages. */
		bdy_map_start = .;
		. += 0x2000;

		*(.bss);
		bss_end = .;
	}

	. = ALIGN(0x4000);
	ptab_start = .;
	.ptab : AT (ptab_start - KMODE_VA) {
		k_pd_start = .;
		k_pd_pa = . - KMODE_VA;
		. += 0x4000;
		k_pd_end = .;

		/* k_pt must be a multiple of PAGE_SIZE,
		 * in order that the rest of the code can
		 * utilize subpage sized PTs.
		 */
		k_pt_start = .;
		k_pt_pa = . - KMODE_VA;
		. += 0x1000;
		k_pt_end = .;
		ptab_end = .;
	}

	. = ALIGN(0x1000);
	excpt_start_pa = . - KMODE_VA;

	/* The below allocations are purely VA allocations,
	 * except the exception vector.
	 */

	/* Allow the kernel binary to grow about 4MB. */
	. = ASSERT(. < (KMODE_VA + KRNL_SZ - 0x1000), "kernel too big.");

	. = KMODE_VA + KRNL_SZ - 0x1000;
	mmu_slub_area = .;
	. += 0x1000;

	. = ALIGN(0x100000);
	/* struct page array for RAM. Reserve 1MB VA space to allow us to
	 * be flexible with the size of the struct.
	 */
	ram_map_start = .;
	. += 0x100000;

	. = 0xf0000000;
	vm_slub_start = .;

	. = 0xf8000000;
	vm_slub_end = .;
	vm_dev_start = .;

	/* High Vectors. */
	. = 0xffff0000;
	excpt_start = .;
	.excpt : AT (excpt_start_pa) {
		kernel/excpt.o(.excpt);
	}

}
