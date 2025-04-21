PROJDIRS := boot kernel

.PHONY: test-iso test iso PZOS.bin clean

test-iso: PZOS.iso
	qemu-system-i386 -cdrom PZOS.iso

test: PZOS.bin
	qemu-system-i386 -kernel PZOS.bin

iso: PZOS.iso

PZOS.iso: PZOS.bin
	cp PZOS.bin isodir/boot/PZOS.bin
	grub-mkrescue -o PZOS.iso isodir

PZOS.bin:
	$(MAKE) -C kernel
	cp kernel/PZOS.bin PZOS.bin

clean:
	for dir in $(PROJDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f *.bin *.iso
	rm -f isodir/boot/PZOS.bin
