.PHONY: img test test-efi debug clean

img:
	$(MAKE) -C limine
	$(MAKE) -C kernel
	$(MAKE) PZOS.bin

PZOS.bin: kernel/kernel.elf limine/limine limine.conf bg.jpg
	dd if=/dev/zero of=PZOS.bin bs=1M count=64
	PATH=${PATH}:/usr/sbin:/sbin sgdisk PZOS.bin -n 1:2048:32768 -t 1:EF00
	mformat -F -i PZOS.bin@@1M

	mmd -i PZOS.bin@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i PZOS.bin@@1M kernel/kernel.elf ::/boot/PZOS.bin
	mcopy -i PZOS.bin@@1M limine.conf limine/limine-bios.sys bg.jpg ::/boot/limine
	mcopy -i PZOS.bin@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i PZOS.bin@@1M limine/BOOTIA32.EFI ::/EFI/BOOT

	limine/limine bios-install PZOS.bin

test: img
	qemu-system-x86_64 -m 4G -drive file=PZOS.bin,format=raw

test-efi: img
	qemu-system-x86_64 -m 4G -drive file=PZOS.bin,format=raw -bios /usr/share/OVMF/OVMF_CODE.fd -net none

debug: img
	qemu-system-x86_64 -m 4G -drive file=PZOS.bin,format=raw -s -S
	# gdb -ex "target remote :1234" -ex "file kernel/kernel.elf" -ex "b kernel_main" -ex "c"

clean:
	$(MAKE) -C kernel clean
	rm -f PZOS.bin
