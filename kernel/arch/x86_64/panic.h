#pragma once
#ifndef ARCH_X86_64_PANIC_H
#define ARCH_X86_64_PANIC_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#include <kernel/isr.h>

// As long as `msg` is a string literal, it should hopefully not touch any registers
#define panic(args...)                              \
	do {                                            \
		struct isr_frame_t *ptr;                    \
		asm volatile("push rax\n"                   \
					 "push rbx\n"                   \
					 "push rcx\n"                   \
					 "push rdx\n"                   \
					 "push rsi\n"                   \
					 "push rdi\n"                   \
					 "push rbp\n"                   \
					 "push r8\n"                    \
					 "push r9\n"                    \
					 "push r10\n"                   \
					 "push r11\n"                   \
					 "push r12\n"                   \
					 "push r13\n"                   \
					 "push r14\n"                   \
					 "push r15\n"                   \
					 "mov %0, rsp\n" : "=rm"(ptr)); \
		_panic(ptr, args);                          \
		__builtin_unreachable();                    \
	} while (0)

__attribute__((noreturn)) void _panic(struct isr_frame_t *, const char *msg, ...);

__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("hlt");
	}
}

#endif
