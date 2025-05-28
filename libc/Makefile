CC := i686-elf-gcc
AS := nasm

CSRCS := $(shell find . -name "*.c")
ASMSRCS := $(shell find . -name "*.asm")
OBJS = $(ASMSRCS:.asm=_asm.o) $(CSRCS:.c=_c.o)

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I../include
ASFLAGS := -f elf32 -Werror

ifeq ($(TYPE),K)
	CFLAGS += -D__is_libk=1 -U__is_libc
else
	CFLAGS += -D__is_libc=1 -U__is_libk
endif

.PHONY: all clean

all: libc.a libk.a

libk.a: $(OBJS)
	$(AR) rcs libk.a $(OBJS)

libc.a: $(OBJS)
	$(AR) rcs libc.a $(OBJS)

%_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS) libk.a libc.a
