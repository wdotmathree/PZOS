#include <kernel/panic.h>

#include <stdio.h>

#include <kernel/tty.h>

__attribute__((noreturn)) void _panic(const char *msg, struct isr_frame_t *frame) {
	if (msg != NULL) {
		tty_puts("\nKernel panic: ");
		tty_puts(msg);
		tty_putchar('\n');
	} else {
		tty_puts("Kernel panic: Unknown error.\n");
	}

	/// TODO: Printstack trace
	if (frame != NULL) {
		tty_puts("Register state:\n");
		printf("RAX: %p RBX: %p RCX: %p RDX: %p\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
		printf("RSI: %p RDI: %p RBP: %p R8:  %p\n", frame->rsi, frame->rdi, frame->rbp, frame->r8);
		printf("R9:  %p R10: %p R11: %p R12: %p\n", frame->r9, frame->r10, frame->r11, frame->r12);
		printf("R13: %p R14: %p R15: %p\n", frame->r13, frame->r14, frame->r15);
		printf("RIP: %p CS:  %#04x             RFLAGS: %#08x\n", frame->isr_rip, (unsigned int)frame->isr_cs, (unsigned int)frame->isr_rflags);
		printf("RSP: %p SS:  %#04x\n", frame->isr_rsp, (unsigned int)frame->isr_ss);
		uint64_t cr0, cr2, cr3, cr4;
		asm("mov %0, cr0;"
			"mov %1, cr2;"
			"mov %2, cr3;"
			"mov %3, cr4;"
			: "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4));
		printf("CR0: %p CR2: %p CR3: %p CR4: %p\n", cr0, cr2, cr3, cr4);
	}

	halt();
}
