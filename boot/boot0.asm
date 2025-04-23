BITS 16

extern boot1

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
	mov ah, 0x00
	int 0x13
	jc error.reset

	; Load rest of bootloader
	mov ah, 0x42
	mov si, diskaddr
	int 0x13
	jc error.read

	; Check A20
	call test_a20
	jne .done_a20
	; Try using BIOS function
	mov ax, 0x2401
	int 0x15
	call test_a20
	jne .done_a20
	; Try using port I/O
	in al, 0xee
	xor ax, ax
	.a20_loop1:
		call test_a20
		jne .done_a20
		dec ax
		jnz .a20_loop1
	; Try using other port
	in al, 0x92
	test al, 2
	jnz .done_a20_fast
	or al, 2
	and al, 0xfe
	out 0x92, al
	.done_a20_fast:
	; Check A20 again, if it still fails, we have to give up
	call test_a20
	je error.a20
	.done_a20:

	; Get memory map
	mov eax, 0xe820
	mov edx, 0x534d4150
	xor ebx, ebx
	mov ecx, 24
	mov di, 0x0508
	.mmap_loop:
		mov dword [di+20], 0
		int 0x15
		jc error.mmap
		add di, 24
		; mov edx, eax
		mov eax, 0xe820
		mov ecx, 24
		test bx, bx
		jnz .mmap_loop
	mov [0x0500], di

	; Load temporary GDT
	lgdt [gdt_ref]

	; Enable protected mode
	mov eax, cr0
	or al, 1
	mov cr0, eax

	jmp 0x0008:thunk

BITS 32
thunk:
	; Set up segment registers
	mov ax, 0x0010
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
	; Fix stack
	movzx esp, sp

	.loop:
		call boot1
		jmp .loop

BITS 16

test_a20:
	push ds
	push es
	push ax
	cli

	xor ax, ax
	mov es, ax
	not ax
	mov ds, ax
	
	mov di, 0x0500
	mov si, 0x0510

	mov al, byte [es:di]
	push ax
	mov al, byte [ds:si]
	push ax

	mov byte [es:di], 0x00
	mov byte [ds:si], 0xff

	cmp byte [es:di], 0xff

	pop ax
	mov byte [ds:si], al
	pop ax
	mov byte [es:di], al

	pop ax
	pop es
	pop ds
	ret

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
.a20:
	mov si, a20_msg
	call write_string
	jmp error.loop
.mmap:
	mov si, mmap_msg
	jmp error

section .data

diskaddr:
	.size db 0x10
	.res db 0
	.numb dw 1
	.buf dd 0x00007e00
	.lba dq 1
reset_msg: db "Disk reset failed with error ", 0
read_msg: db "Disk read failed with error ", 0
a20_msg: db "Failed to enable A20 line", 0
mmap_msg: db "Failed to get memory map", 0
tmp_gdt:
	.null dq 0
	.top1 dw 0xffff, 0x0000
	.bottom1 db 0x00, 0x9a, 0xcf, 0x00
	.top2 dw 0xffff, 0x0000
	.bottom2 db 0x00, 0x92, 0xcf, 0x00
gdt_ref:
	.size dw $ - tmp_gdt - 1
	.addr dd tmp_gdt
