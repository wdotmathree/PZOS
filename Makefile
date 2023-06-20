PROJDIRS := kernel # user

.PHONY: test iso clean $(PROJDIRS)

test: kernel.bin
	qemu-system-i386 -kernel kernel.bin

test-iso: kernel.iso
	# qemu-system-i386 -drive format=raw,file=kernel.iso -boot order=c
	qemu-system-i386 -cdrom kernel.iso

kernel.iso: kernel.bin
	cp kernel.bin isodir/boot/kernel.bin
	grub-mkrescue -o kernel.iso isodir

kernel.bin: $(PROJDIRS)
	i686-elf-gcc -T linker.ld -o kernel.bin -ffreestanding -O2 -nostdlib \
		$(foreach dir, $(PROJDIRS), $(wildcard $(dir)/*.o)) -lgcc

$(PROJDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(PROJDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f *.bin
