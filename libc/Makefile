CSRCS := $(shell find . -name "*.c")
ASMSRCS := $(shell find . -name "*.asm")

CC := i686-elf-gcc
AS := nasm

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I. -I../libc/include -D_KERNEL
ASFLAGS := -f elf32

ifeq ($(TYPE),K)
	OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o) $(wildcard ../libc/*-libk.o)
	CFLAGS += -D_KERNEL -U_USER
else
	OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o) $(wildcard ../libc/*-libu.o)
	CFLAGS += -D_USER -U_KERNEL
endif

.PHONY: all clean

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS)
