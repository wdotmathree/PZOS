SECTIONS {
	.mbr 0x7c00 : AT(0x0000) {
		boot0.o (.text)
		boot0.o (.data)
	}
	.sig 0x1fe : AT(0x1fe) {
		SHORT(0xaa55)
	}
	.boot 0x7e00 : AT(0x0200) {
		*(.text)
		*(.data)
	}
}
