AS := nasm
CC := /opt/cross/bin/i686-elf-gcc
LD := /opt/cross/bin/i686-elf-ld

ASMSRCS := $(wildcard *.asm)
CSRCS := $(wildcard *.c)
OBJS = $(ASMSRCS:.asm=_asm.o) $(CSRCS:.c=_c.o)

# ASMLIBS = $(shell cd ../libc ; find *.asm)
CLIBS = $(shell cd ../libc ; find *.c)
LIBOBJS = $(ASMLIBS:.asm=_lasm.o) $(CLIBS:.c=_lc.o)

ASFLAGS := -f elf32
CFLAGS := -std=gnu99 -ffreestanding -Wall -Wextra -Wno-unused-parameter -I../include -mgeneral-regs-only -mno-red-zone -masm=intel

.PHONY: release debug clean

release: CFLAGS += -O2
release: boot.elf

test: release
	qemu-system-i386 -m 8G -drive file=combined.bin,format=raw

debug: CFLAGS += -g -Og
debug: ASFLAGS += -gdwarf
debug: boot.elf
	qemu-system-i386 -m 8G -drive file=boot.bin,format=raw -s -S
	# gdb -ex "target remote :1234" -ex "file boot.elf" -ex "b boot1" -ex "c"

boot.elf: $(OBJS) $(LIBOBJS)
	$(LD) -T link.ld -o boot.elf $(OBJS) $(LIBOBJS)
	objcopy -O binary -j .mbr boot.elf mbr.bin
	objcopy -S -O binary -R .mbr boot.elf boot.bin

# Real mode and others
%_asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Protected mode
%_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_lasm.o: ../libc/%.asm
	$(AS) $(ASFLAGS) $< -o $@

%_lc.o: ../libc/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIBOBJS) boot.elf boot.bin mbr.bin
