.PHONY: PZOS.bin test clean

PZOS.bin:
	make -C kernel
	make -C boot
	mkdir -p imgdir/sys
	dd if=/dev/zero of=PZOS.bin bs=1M count=64
	mformat -F -i PZOS.bin
	cp kernel/kernel.bin imgdir/sys/kernel.bin
	mcopy -i PZOS.bin imgdir/* ::
	dd if=boot/mbr.bin of=PZOS.bin bs=512 count=1 conv=notrunc
	dd if=boot/boot.bin of=PZOS.bin bs=512 seek=1 conv=notrunc
	
test: PZOS.bin
	qemu-system-i386 -drive file=PZOS.bin,format=raw

debug: PZOS.bin
	qemu-system-i386 -m 8G -drive file=PZOS.bin,format=raw -s -S
	# gdb -ex "target remote :1234" -ex "file kernel/kernel.elf" -ex "b kernel_main" -ex "c"

clean:
	make -C boot clean
	make -C kernel clean
	rm -f *.bin *.iso
	rm -f isodir/boot/PZOS.bin
