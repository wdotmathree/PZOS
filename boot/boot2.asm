section .text
extern exception_handler

%assign i 0
%rep 32
	isr_err_stub_%+i:
		push i
		call exception_handler
		add esp, 4
		iret
%assign i i+1
%endrep

section .data
global idt32
global idtr32

align 16
idt32:
%assign i 0
%rep 32
	.entry%+i dw isr_err_stub_%+i
	dw 0x08 ; code segment selector
	db 0x00 ; reserved
	db 0x8e ; present, ring 0, interrupt gate
	dw 0 ; high bits should be zero
%assign i i+1
%endrep
	.extra times 16 * 16 db 0 ; reserve space for 16 extra entries
idtr32:
	.limit dw $ - idt32 - 1
	.base dd idt32
