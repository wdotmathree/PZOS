AS := nasm
CC := /opt/cross/bin/i686-elf-gcc
LD := /opt/cross/i686-elf/bin/ld

ASMSRCS := $(wildcard *.asm)
CSRCS := $(wildcard *.c)
SRCS := $(ASMSRCS) $(CSRCS)
OBJS = $(ASMSRCS:.asm=_asm.o) $(CSRCS:.c=_c.o)

ASFLAGS := -f elf32 -gdwarf
CFLAGS := -ffreestanding -I../include/ -mgeneral-regs-only -mno-red-zone -masm=intel

.PHONY: all clean

release: CFLAGS += -O2
release: all

all: $(OBJS)
	$(LD) -m elf_i386 -T link.ld -o boot.elf $(OBJS) -L../libc -lk
	objcopy -S -O binary boot.elf boot.bin

debug: CFLAGS += -g -Og
debug: all
	qemu-system-i386 -m 8G -drive file=boot.bin,format=raw -s -S
	# gdb -ex "target remote :1234" -ex "file boot.elf" -ex "b boot1" -ex "c"

test: release
	qemu-system-i386 -m 8G -drive file=boot.bin,format=raw

# Long mode
boot2_c.o: boot2.c
	/opt/cross/bin/x86_64-elf-gcc $(CFLAGS) -c $< -o $@

# Real mode and others
%_asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Protected mode
%_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) boot.elf boot.bin
