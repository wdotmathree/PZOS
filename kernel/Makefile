.PHONY: release debug all clean

ASMSRCS := $(shell find src arch/$(ARCH) -name "*.asm")
CSRCS := $(shell find src arch/$(ARCH) -name "*.c")
OBJS := $(CSRCS:.c=_c.o) $(ASMSRCS:.asm=_asm.o)

ASFLAGS := -f elf64
CFLAGS := -std=gnu11 -Wall -Wextra -Wno-unused-parameter -I./include -I./arch/$(ARCH) -I../libc/include -I../limine -mno-red-zone -masm=intel -ffreestanding -mcmodel=kernel -mgeneral-regs-only

include arch/$(ARCH)/make.config

release: CFLAGS += -O2
release: kernel.elf
	objcopy -S kernel.elf kernel.bin

debug: CFLAGS += -g -O0
debug: ASFLAGS += -gdwarf
debug: TARGET := debug
debug: kernel.elf
	cp kernel.elf kernel.bin

kernel.elf: $(OBJS) link.ld ../libc/libk.a
	$(CC) -T link.ld -o kernel.elf $(OBJS) -L../libc -nostdlib -lk -lgcc

../libc/libk.a: $(shell find ../libc -name "*.c" -o -name "*.asm")
	$(MAKE) -C ../libc $(TARGET)

src/tty.o: glyphs.bin
%_c.o: %.c Makefile $(shell find ../libc/include ./include ./arch/$(ARCH) ../limine -name "*.h")
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.asm Makefile
	$(AS) $(ASFLAGS) $< -o $@

clean:
	$(MAKE) -C ../libc clean
	rm -f $(OBJS) kernel.elf kernel.bin
