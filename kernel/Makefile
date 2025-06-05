AS := nasm
CC := /opt/cross/bin/x86_64-elf-gcc
LD := /opt/cross/bin/x86_64-elf-ld

ASMSRCS := $(wildcard *.asm)
CSRCS := $(wildcard *.c)
OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o)

ASFLAGS := -f elf64
CFLAGS := -std=gnu99 -ffreestanding -Wall -Wextra -Wno-unused-parameter -I../include -mno-red-zone -masm=intel -D_KERNEL

.PHONY: release debug all libk clean

release: CFLAGS += -O2
release: kernel.elf
	objcopy -S -R .eh_frame -R .comment kernel.elf kernel.bin

debug: CFLAGS += -g -Og
debug: ASFLAGS += -gdwarf
debug: kernel.elf
	cp kernel.elf kernel.bin

kernel.elf: libk $(OBJS)
	$(LD) -T link.ld -o kernel.elf $(OBJS) -L../libc -lk

libk:
	TYPE=K $(MAKE) -C ../libc libk.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	TYPE=K $(MAKE) -C ../libc clean
	rm -f $(OBJS) kernel.bin
