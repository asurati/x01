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
CROSS := ~/cross/arm-none-eabi/tools


# RPI2 for QEMU emulation, RPI for hardware execution.
BOARD := QRPI2
#BOARD := RPI

QEMU :=	qemu-system-arm
CC := LD_LIBRARY_PATH=$(CROSS)/lib $(CROSS)/bin/arm-none-eabi-gcc
LD := $(CROSS)/bin/arm-none-eabi-ld
AS := $(CROSS)/bin/arm-none-eabi-as
OC := $(CROSS)/bin/arm-none-eabi-objcopy
DD := dd
MK := mkimage

ELF := pi.elf
BIN := pi.bin
GZBIN := $(BIN).gz
HWIMG := hw.img

OBJS := boot/start.o
OBJS += boot/boot.o

OBJS += lib/string.o
OBJS += lib/assert.o
OBJS += lib/div.o

OBJS += kernel/excpt.o
OBJS += kernel/sched.o
OBJS += kernel/mmu.o
OBJS += kernel/bdy.o
OBJS += kernel/pm.o
OBJS += kernel/slub.o
OBJS += kernel/list.o
OBJS += kernel/vm.o
OBJS += kernel/io.o
OBJS += kernel/excptc.o
OBJS += kernel/irq.o
OBJS += kernel/schedc.o
OBJS += kernel/fb.o
OBJS += kernel/ioreq.o
OBJS += kernel/work.o
OBJS += kernel/main.o

OBJS += dev/timer.o
OBJS += dev/intc.o
OBJS += dev/mbox.o
OBJS += dev/uart.o
OBJS += dev/sdhc.o
OBJS += dev/sd.o
ifeq ($(BOARD),QRPI2)
OBJS += dev/timer_qrpi2.o
else
OBJS += dev/timer_rpi1.o
endif

#-d int,guest_errors,unimp -display none

QFLAGS := -m 256 -M raspi2 -kernel $(BIN) -serial stdio \
	  -d int,guest_errors,unimp \
	  -drive file=img.qcow2,if=sd,format=qcow2 \
	  -monitor tcp::4444,server,nowait

LDFLAGS := -n -T pi.ld -defsym LOAD_PA=$(LOAD_PA)
CFLAGS := -c -mcpu=arm1176jzf-s -ffreestanding -nostdlib -O3 -Wall -Wextra \
	  -Werror -I ./include/ -fno-common -D$(BOARD) -g -mabi=aapcs \
	  -mno-unaligned-access
AFLAGS := -mcpu=arm1176jzf-s

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

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c	
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BIN) $(GZBIN) $(ELF) $(HWIMG) $(OBJS)

rundbg:
	$(QEMU) $(QFLAGS) -s -S

run:
	$(QEMU) $(QFLAGS)

.PHONY: all clean run rundbg
