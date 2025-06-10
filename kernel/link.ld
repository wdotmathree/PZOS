ENTRY(kinit)

PHDRS {
	limine_requests PT_LOAD;
	text PT_LOAD;
	rodata PT_LOAD;
	data PT_LOAD;
}

SECTIONS {
	. = 0xffffffff80000000;

	.limine_requests : ALIGN(0x1000) {
		*(.limine_requests_start)
		*(.limine_requests)
		*(.limine_requests_end)
	} : limine_requests

	.text BLOCK(0x1000) : ALIGN(0x1000) {
		*(.text)
	} : text

	.rodata BLOCK(0x1000) : ALIGN(0x1000) {
		*(.rodata*)
	} : rodata

	.data BLOCK(0x1000) : ALIGN(0x1000) {
		*(.data)
	} : data

	.bss BLOCK(0x1000) : ALIGN(0x1000) {
		*(COMMON)
		*(.bss)
	} : data

	/DISCARD/ : {
		*(.eh_frame)
		*(.comment)
		*(.note.GNU-stack)
	}

	_binary_size = (. - 0xffffffff7ffff001) & 0xfffffffffffff000;
}
