SECTIONS {
	.mbr 0x7c00 : AT(0x0000) {
		mbr* (.text .data .bss .rodata*)
	}
	.text 0x7e00 : AT(0x0200) { *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	.data : { *(.data .data.*) }
	.bss : { *(.bss .bss.*) }
	_binary_end = .;
}
