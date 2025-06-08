.PHONY: release debug all clean

AS := nasm
CC := /opt/cross/bin/x86_64-elf-gcc
LD := /opt/cross/bin/x86_64-elf-ld

ASMSRCS := $(wildcard *.asm)
CSRCS := $(wildcard *.c)
OBJS := $(CSRCS:.c=_c.o) $(ASMSRCS:.asm=_asm.o)

ASFLAGS := -f elf64
CFLAGS ?= -mgeneral-regs-only -std=gnu11 -mcmodel=kernel -ffreestanding -Wall -Wextra -Wno-unused-parameter -I../include -I../limine -mno-red-zone -masm=intel

release: CFLAGS += -O2
release: all
	objcopy -S kernel.elf kernel.bin

debug: CFLAGS += -g -O0
debug: ASFLAGS += -gdwarf
debug: TARGET := debug
debug: all
	cp kernel.elf kernel.bin

all:
	$(MAKE) -C ../libc $(TARGET)
	CFLAGS="$(CFLAGS)" $(MAKE) kernel.elf

kernel.elf: $(OBJS) link.ld ../libc/libk.a
	$(LD) -T link.ld -o kernel.elf $(OBJS) -L../libc -lk

tty.o: glyphs.bin
%_c.o: %.c Makefile ../include/**
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

clean:
	$(MAKE) -C ../libc clean
	rm -f $(OBJS) kernel.elf kernel.bin
