section .multiboot
align 4
dd 0x1badb002
dd 0x00000003
dd -(0x1badb002 + 0x00000003)

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
extern kernel_main
global _start:(_start.end - _start)

_start:
	mov esp, stack_top
	
	call kernel_main

	cli
	hlt
	db 0xeb, 0xfd ; jmp byte -3 (to hlt)

.end:
