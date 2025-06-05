AS := nasm
CC := /opt/cross/bin/x86_64-elf-gcc

ASMSRCS := $(shell find . -name "*.asm")
CSRCS := $(shell find . -name "*.c")

CFLAGS := -std=gnu99 -ffreestanding -Wall -Wextra -Wno-unused-parameter -I../include
ASFLAGS := -f elf64

ifeq ($(TYPE),K)
	CFLAGS += -D__is_libk=1 -U__is_libc
	OBJS = $(ASMSRCS:.asm=_K_asm.o) $(CSRCS:.c=_K_c.o)

%_K_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_K_asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

else
	CFLAGS += -D__is_libc=1 -U__is_libk
	OBJS = $(ASMSRCS:.asm=_C_asm.o) $(CSRCS:.c=_C_c.o)

%_C_c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_C_asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

endif

.PHONY: release debug clean

release: CFLAGS += -O2
release: libc.a

debug: CFLAGS += -g -Og
debug: ASFLAGS += -gdwarf
debug: libc.a

libk.a: $(OBJS)
	$(AR) rcs libk.a $(OBJS)

libc.a: $(OBJS)
	$(AR) rcs libc.a $(OBJS)

clean:
	rm -f *.o libk.a libc.a
