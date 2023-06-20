PROJDIRS := kernel # user

.PHONY: clean

test: kernel.bin
	qemu-system-x86_64 -kernel kernel.bin

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

.PHONY: all clean $(PROJDIRS)
