SECTIONS {
	.mbr 0x7c00 : AT(0x0000) {
		mbr* (.text .data .bss .rodata*)
	}
	.text 0x7e00 : AT(0x0200) { *(.text .text.*) }
	.data : { *(.data .data.*) }
	.bss : { *(.bss .bss.*) }
	.rodata : { *(.rodata .rodata.*) }
	_binary_end = .;
}
