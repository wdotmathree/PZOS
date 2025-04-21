AS := nasm
CC := /opt/cross/bin/i686-elf-gcc
LD := /opt/cross/i686-elf/bin/ld

ASMSRCS := $(wildcard *.asm)
CSRCS := $(wildcard *.c)
SRCS := $(ASMSRCS) $(CSRCS)
OBJS = $(ASMSRCS:.asm=.o) $(CSRCS:.c=.o)

ASFLAGS := -felf -gdwarf

.PHONY: all clean

all: $(OBJS)
	$(LD) -m elf_i386 -T link.ld -o boot.elf $(OBJS)
	objcopy -S boot.elf boot.stripped.elf
	objcopy -O binary boot.stripped.elf boot.bin

debug: all
	qemu-system-i386 -m 8G -drive file=boot.bin,format=raw -s -S
	# gdb -ex "target remote :1234" -ex "file boot.elf" -ex "b stage2" -ex "c"

test: all
	qemu-system-i386 -m 8G -drive file=boot.bin,format=raw

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) -c -m32 -ffreestanding -g -I. $< -o $@

clean:
	rm -f $(OBJS) boot.elf boot.bin
