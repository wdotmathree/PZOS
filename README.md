# PZOS
Amateur x86-64 (for now) kernel written from scratch using C and NASM assembly, booted using Limine.

Currently only boots into a basic terminal application in kernel space but more features are to come soon.

## Features
- Project structure built with future multiarch support in mind
- UEFI ready and legacy BIOS compatible
- ACPI table reading support
- Serial interface for debugging
- Interrupt driven PS/2 keyboard driver
- Flexible keyboard stack ready for future USB integration
- Modular design and simple interfaces making for easy optimization in the future

## Usage

1. Clone the repo
```bash
git clone https://github.com/wdotmathree/PZOS.git
git submodule update --init --recursive --progress
```

2. Build the image file

```bash
make img -j$(nproc)
```

### Using an emulator:

3. Install prerequisites
```bash
sudo apt install qemu-system-x86

# For UEFI support (optional)
sudo apt install ovmf
```

4. Run the image with QEMU
```bash
# 4GB RAM max memory size (not allocated at boot), adjust as needed
# If using UEFI (PS/2 is not supported by the UEFI firmware so keyboard won't work)
qemu-system-x86_64 -m 4G -drive format=raw,file=PZOS.bin -bios /usr/share/OVMF/OVMF_CODE.fd

# If using legacy BIOS
qemu-system-x86_64 -m 4G -drive format=raw,file=PZOS.bin
```

### Booting from USB:

3. Find the correct device path for your USB drive. Run `lsblk` before and after plugging in the USB drive to identify it (e.g., /dev/sdX).

4. Format the USB drive into 2 partitions:
   - A small EFI partition formatted as FAT
   - A larger FAT32 (for now) partition for the root filesystem

5. Write the image to a USB drive (replace /dev/sdX with your USB drive)
```bash
# WARNING: This will erase all data on the target drive!
# Make sure you are absolutely certain you have the correct drive.
# We are not responsible for any data loss.
make file=/dev/sdX install -j$(nproc)
```

6. Plug the USB into a computer, then reboot it off the USB drive. (very system specific, search online for instructions if unsure)

## Contributing

Feel free to open issues for any bugs you find, however I am not currently accepting pull requests as this is a personal project.

## License

This project is licensed under the GNU AGPLv3 License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [Limine](https://github.com/limine-bootloader/limine) - The bootloader used in this project
- [OSDev Wiki](https://wiki.osdev.org/Main_Page) - A great resource for OS development
- [Intel 64 and IA-32 Architectures Software Developer’s Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - The definitive guide to x86 architecture (specifically Volume 3)
