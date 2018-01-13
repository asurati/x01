#
# Copyright (c) 2018 Amol Surati
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# LOAD_PA must be 0x10000, if QEMU is to run the binary.
LOAD_PA := 0x10000

# RPI2 for QEMU emulation, RPI for hardware execution.
BOARD := QRPI2
#BOARD := RPI

QEMU :=	qemu-system-arm
CC := arm-none-eabi-gcc
LD := arm-none-eabi-ld
AS := arm-none-eabi-as
OC := arm-none-eabi-objcopy
DD := dd
MK := mkimage

ELF := pi.elf
TBIN := pi.tmp.bin
BIN := pi.bin
GZBIN := $(BIN).gz
HWIMG := hw.img
SWIMG := sw.img

QFLAGS := -m 256 -M raspi2 -kernel $(BIN) -serial stdio \
	  -d int,guest_errors,unimp -display none \
	  -monitor tcp::4444,server,nowait

LDFLAGS := -n -T pi.ld -defsym LOAD_PA=$(LOAD_PA)
CFLAGS := -c -mcpu=arm1176jzf-s -ffreestanding -nostdlib -O3 -Wall -Wextra \
	  -Werror -I ./include/ -fno-common -D$(BOARD) -g
AFLAGS := -mcpu=arm1176jzf-s

OBJS := start.o boot.o main.o mmu.o bdy.o pm.o atomic.o slub.o
OBJS += list.o vm.o
OBJS += lib/string.o lib/assert.o

IMG_ENTRY = 0x$(shell xxd -l 4 -s 0x18 -e $(ELF) | cut -c11-18)

all: $(HWIMG)

$(HWIMG): $(GZBIN)
	$(MK) -A arm -O linux -T kernel -C gzip \
		-n $@ -a $(LOAD_PA) -e $(IMG_ENTRY) -d $< $@

$(GZBIN): $(BIN)
	gzip -fk $<
	    
$(BIN): $(ELF)
	$(OC) -O binary $< $@

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.S
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c	
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BIN) $(TBIN) $(GZBIN) $(ELF) $(HWIMG) $(SWIMG) $(OBJS)

rundbg:
	$(QEMU) $(QFLAGS) -s -S

run:
	$(QEMU) $(QFLAGS)

.PHONY: all clean run rundbg
