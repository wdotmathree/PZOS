CC := i686-elf-gcc
AS := nasm

CSRCS := $(wildcard *.c)
ASMSRCS := $(wildcard *.asm)
OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o)

CRTBEGIN := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I./include -I../libc/include -D_KERNEL
ASFLAGS := -f elf32 -Werror

.PHONY: all crtfiles libk clean

all: libk $(OBJS) crtfiles
	$(CC) -T linker.ld -o PZOS.bin -ffreestanding -O2 -nostdlib crt/crti.o $(CRTBEGIN) $(OBJS) $(CRTEND) crt/crtn.o -L../libc -lk -lgcc

crtfiles: crt/crti.o crt/crtn.o

libk:
	TYPE=K $(MAKE) -C ../libc libk.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	TYPE=K $(MAKE) -C ../libc clean
	rm -f $(OBJS)
	rm -f crt/crti.o crt/crtn.o
