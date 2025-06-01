BITS 16

section .text

boot:
	; Canonicalize IP
	jmp 0:.canon
	.canon:

	; Disable interrupts
	cli

	; Set up segment registers
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	; Set up stack
	mov sp, 0x7c00

	; Reset disk
	; dl should be set by BIOS
	mov ah, 0x00
	int 0x13
	jc error.reset

	; Load rest of bootloader
	mov ax, [diskaddr.numb]
	shr ax, 9
	mov [diskaddr.numb], ax
	mov ah, 0x42
	mov si, diskaddr
	int 0x13
	jc error.read

	jmp 0x0000:0x7e00

write_string:
	pusha
	mov ah, 0x0e
	.loop:
		lodsb
		test al, al
		jz .done
		int 0x10
		jmp .loop
	.done:
	popa
	ret

printint:
	cwd
	mov bx, 10
	div bx
	push dx
	test ax, ax
	jz .done
	call printint
	.done:
	pop ax
	add al, 0x30
	mov ah, 0x0e
	xor bh, bh
	int 0x10
	ret

error:
	call write_string
	shr ax, 4
	call printint
	.loop:
		cli
		hlt
		jmp .loop
.reset:
	mov si, reset_msg
	jmp error
.read:
	mov si, read_msg
	jmp error

section .data

extern _binary_end

diskaddr:
	.size db 0x10
	.res db 0
	.numb dw _binary_end - 0x7c00 - 1
	.buf dd 0x00007e00
	.lba dq 1
reset_msg: db "Disk reset failed with error ", 0
read_msg: db "Disk read failed with error ", 0
