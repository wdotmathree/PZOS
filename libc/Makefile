CSRCS := $(shell find . -name "*.c")
ASMSRCS := $(shell find . -name "*.asm")

CC := i686-elf-gcc
AS := nasm

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I./include -I../kernel/include -D_KERNEL
ASFLAGS := -f elf32 -Werror

ifeq ($(TYPE),K)
	OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o)
	CFLAGS += -D__is_libk=1 -U__is_libc
else
	OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.asm=.o)
	CFLAGS += -D__is_libc=1 -U__is_libk
endif

.PHONY: all clean

libk.a: $(OBJS)
	$(AR) rcs libk.a $(OBJS)

libc.a: $(OBJS)
	$(AR) rcs libc.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS) libk.a libc.a
