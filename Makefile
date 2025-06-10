.PHONY: img test debug clean

export AS := nasm
export AR := /opt/cross/bin/x86_64-elf-ar
export CC := /opt/cross/bin/x86_64-elf-gcc
export ARCH := x86_64

img:
	$(MAKE) -C limine
	$(MAKE) -C kernel $(TARGET)

	$(MAKE) PZOS.bin

PZOS.bin: Makefile kernel/kernel.elf limine/limine limine.conf bg.jpg
	dd if=/dev/zero of=PZOS.bin bs=1M count=64
	PATH=${PATH}:/usr/sbin:/sbin sgdisk PZOS.bin -n 1:2048:32767 -t 1:EF00 -n 2:32768 -t 2:8300
	mformat -i PZOS.bin@@1M -T 30720
	mformat -F -i PZOS.bin@@16M

	mmd -i PZOS.bin@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i PZOS.bin@@1M kernel/kernel.elf ::/boot/PZOS.bin
	mcopy -i PZOS.bin@@1M limine.conf limine/limine-bios.sys bg.jpg ::/boot/limine
	mcopy -i PZOS.bin@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i PZOS.bin@@1M limine/BOOTIA32.EFI ::/EFI/BOOT

	limine/limine bios-install PZOS.bin

test: img
	qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -bios /usr/share/OVMF/OVMF_CODE.fd -net none -serial mon:stdio -nographic

debug: TARGET := debug
debug: img
	qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -bios /usr/share/OVMF/OVMF_CODE.fd -net none -serial mon:stdio -nographic -s -S
	# gdb -ex "target remote :1234" -ex "file kernel/kernel.elf" -ex "b kmain" -ex "c"

clean:
	$(MAKE) -C kernel clean
	rm -f PZOS.bin
