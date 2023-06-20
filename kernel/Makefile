CSRCS := $(wildcard *.c)
ASMSRCS := $(wildcard *.asm)
LIBOBJS := $(wildcard ../libc/*-libk.o)
OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o)

CC := i686-elf-gcc
AS := nasm

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I. -I../libc/include -D_KERNEL
ASFLAGS := -f elf32

.PHONY: all clean libk

all: libk $(OBJS)

libk:
	TYPE=K $(MAKE) -C ../libc

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS)
	TYPE=K $(MAKE) -C ../libc clean
