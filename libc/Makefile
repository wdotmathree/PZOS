.PHONY: release debug clean

AS := nasm
AR := /opt/cross/bin/x86_64-elf-ar
CC := /opt/cross/bin/x86_64-elf-gcc

ASMSRCS := $(shell find . -name "*.asm")
CSRCS := $(shell find . -name "*.c")

ASFLAGS := -f elf64
CFLAGS := -std=gnu11 -Wall -Wextra -Wno-unused-parameter -I../include -mno-red-zone -masm=intel

OBJS := $(CSRCS:.c=_c.o) $(ASMSRCS:.asm=_asm.o)
KOBJS := $(OBJS:.o=.k.o)

release: CFLAGS += -O2
release: libk.a # libc.a

debug: CFLAGS += -g -Og
debug: ASFLAGS += -gdwarf
debug: libk.a # libc.a

libk.a: $(KOBJS)
	$(AR) rcs libk.a $(KOBJS)

libc.a: $(COBJS)
	$(AR) rcs libc.a $(COBJS)

%_c.o: %.c Makefile ../include/**
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

%_c.k.o: %.c Makefile ../include/**
	$(CC) $(CFLAGS) -c $< -o $@ -D__is_libk -ffreestanding -I../limine -mcmodel=kernel -mgeneral-regs-only

%_asm.k.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@ -D__is_libk

clean:
	rm -f $(OBJS) $(KOBJS) libk.a libc.a
