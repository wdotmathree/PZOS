SECTIONS {
	.mbr 0x7c00 : AT(0x0000) {
		boot0* (.text .data)
	}
	.sig 0x1fe : AT(0x1fe) {
		SHORT(0xaa55)
	}
	.boot 0x7e00 : AT(0x0200) {
		boot1* (.text .data .bss .rodata*)
	}
	.text : { *(.text .text.*) }
	.data : { *(.data .data.*) }
	.bss : { *(.bss .bss.*) }
	.rodata : { *(.rodata .rodata.*) }
	_binary_end = .;
}
