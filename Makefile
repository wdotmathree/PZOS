.PHONY: img test debug clean

export AS := nasm
export AR := /opt/cross/bin/x86_64-elf-ar
export CC := /opt/cross/bin/x86_64-elf-gcc
export ARCH := x86_64

img:
	$(MAKE) -C limine
	$(MAKE) -C kernel $(TARGET)

	$(MAKE) PZOS.bin

MOUNTPOINT := /tmp/pzos-mnt
install:
	@if [ -z "$(file)" ] || [ ! -e "$(file)" ] || [ ! -e "$(file)1" ] || [ ! -e "$(file)2" ]; then \
		echo "Usage: $(MAKE) file=/dev/device install"; \
		echo "       Device must be partitioned with an EFI partition (FAT) and a root partition (FAT32)\n"; \
		exit 1; \
	fi
	$(MAKE) -C limine
	$(MAKE) -C kernel

	# mkfs.fat -F16 $(file)1
	# mkfs.fat -F32 $(file)2

	mkdir -p $(MOUNTPOINT)

	sudo mount $(file)1 $(MOUNTPOINT)
	mkdir -p $(MOUNTPOINT)/boot/limine
	mkdir -p $(MOUNTPOINT)/EFI/BOOT
	sudo cp kernel/kernel.elf $(MOUNTPOINT)/boot/PZOS.bin
	sudo cp limine.conf $(MOUNTPOINT)/boot/limine
	sudo cp limine/limine-bios.sys $(MOUNTPOINT)/boot/limine
	sudo cp limine/BOOTX64.EFI $(MOUNTPOINT)/EFI/BOOT
	sudo cp limine/BOOTIA32.EFI $(MOUNTPOINT)/EFI/BOOT
	sudo cp bg.jpg $(MOUNTPOINT)/boot/limine
	sudo umount -l $(MOUNTPOINT)

	sudo mount $(file)2 $(MOUNTPOINT)
	@# Copy files from sysroot
	sudo umount -l $(MOUNTPOINT)

	# limine/limine bios-install $(file)

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
	# qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -bios /usr/share/OVMF/OVMF_CODE.fd -serial mon:stdio -nographic
	# qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -monitor stdio
	qemu-system-x86_64 -cpu max -m 8G -drive file=PZOS.bin,format=raw -d int -D logfile -no-reboot
	# qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -nographic -serial file:/dev/stdout

debug: TARGET := debug
debug: img
	# gdb -ex "target remote :1234" -se "kernel/kernel.elf"
	# qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -nographic -serial file:/dev/stdout -s -S
	qemu-system-x86_64 -cpu max -m 4G -drive file=PZOS.bin,format=raw -nographic -serial file:/dev/stdout -s -S

clean:
	$(MAKE) -C kernel clean
	rm -f PZOS.bin
