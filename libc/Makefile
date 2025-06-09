.PHONY: release debug clean

ASMSRCS := $(shell find . -name "*.asm")
CSRCS := $(shell find . -name "*.c")

ASFLAGS := -f elf64
CFLAGS := -std=gnu11 -Wall -Wextra -Wno-unused-parameter -I./include -mno-red-zone -masm=intel

include arch/$(ARCH)/make.config

OBJS := $(CSRCS:.c=_c.o) $(ASMSRCS:.asm=_asm.o)
KOBJS := $(OBJS:.o=.k.o)

release: CFLAGS += -O2
release: libk.a # libc.a

debug: CFLAGS += -g -O0
debug: ASFLAGS += -gdwarf
debug: libk.a # libc.a

libk.a: $(KOBJS)
	$(AR) rcs libk.a $(KOBJS)

libc.a: $(COBJS)
	$(AR) rcs libc.a $(COBJS)

%_c.o: %.c Makefile $(shell find ./include)
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

%_c.k.o: %.c Makefile $(shell find ./include ../kernel/include ../kernel/arch/$(ARCH) ../limine)
	$(CC) $(CFLAGS) -c $< -o $@ -D__is_libk -ffreestanding -I../kernel/include -I../limine -mcmodel=kernel -mgeneral-regs-only

%_asm.k.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@ -D__is_libk

clean:
	rm -f $(OBJS) $(KOBJS) libk.a libc.a
