PROJDIRS := kernel # user

.PHONY: test iso clean $(PROJDIRS)

test: PZOS.bin
	qemu-system-i386 -kernel PZOS.bin

test-iso: PZOS.iso
	# qemu-system-i386 -drive format=raw,file=PZOS.iso -boot order=c
	qemu-system-i386 -cdrom PZOS.iso

iso: PZOS.iso

PZOS.iso: PZOS.bin
	cp PZOS.bin isodir/boot/PZOS.bin
	grub-mkrescue -o PZOS.iso isodir

PZOS.bin: $(PROJDIRS)
	i686-elf-gcc -T linker.ld -o PZOS.bin -ffreestanding -O2 -nostdlib \
		$(foreach dir, $(PROJDIRS), $(wildcard $(dir)/*.o)) -lgcc

$(PROJDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(PROJDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f *.bin *.iso
	rm -f isodir/boot/PZOS.bin
