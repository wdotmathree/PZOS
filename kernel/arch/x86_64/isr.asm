section .text

global isr_stub_0
extern isr_vectors

isr_stub:
	sub rsp, 512
	fxsave [rsp]

	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rdi, rsp
	mov rax, [rsp + 15 * 8] ; Interrupt number
	call [isr_vectors + rax * 8]
	mov rsp, rax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax

	fxrstor [rsp]

	add rsp, 512 + 16
	iretq

%assign i 0
%rep 256
	align 16
	isr_stub_%+i:
		%if !(i == 7 || i == 9 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17)
		push 0
		%endif
		push i
		jmp isr_stub
%assign i i + 1
%endrep
