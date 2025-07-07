#include <kernel/panic.h>

#include <stdarg.h>
#include <stdio.h>

#include <kernel/log.h>
#include <kernel/tty.h>

extern int vprintf(const char *, va_list);

__attribute__((noreturn)) void _panic(struct isr_frame_t *frame, const char *msg, ...) {
	tty_putchar('\n');
	if (msg != NULL) {
		va_list args;
		va_start(args, msg);
		LOGn("Kernel panic", "");
		vprintf(msg, args);
		va_end(args);
	} else {
		LOG(NULL, "Kernel panic: Unknown error.");
	}

	/// TODO: Print stack trace
	if (frame != NULL) {
		tty_puts("\nRegister state:\n");
		printf("RAX: %p RBX: %p RCX: %p RDX: %p\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
		printf("RSI: %p RDI: %p RBP: %p R8:  %p\n", frame->rsi, frame->rdi, frame->rbp, frame->r8);
		printf("R9:  %p R10: %p R11: %p R12: %p\n", frame->r9, frame->r10, frame->r11, frame->r12);
		printf("R13: %p R14: %p R15: %p\n", frame->r13, frame->r14, frame->r15);
		printf("RIP: %p CS:  %#.4x          RFLAGS: %#.8x\n", frame->isr_rip, frame->isr_cs, frame->isr_rflags);
		printf("RSP: %p SS:  %#.4x\n", frame->isr_rsp, frame->isr_ss);
		uint64_t cr0, cr2, cr3, cr4;
		asm("mov %0, cr0\n\t"
			"mov %1, cr2\n\t"
			"mov %2, cr3\n\t"
			"mov %3, cr4"
			: "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4));
		printf("CR0: %p CR2: %p CR3: %p CR4: %p\n", cr0, cr2, cr3, cr4);
	}

	hcf();
}
