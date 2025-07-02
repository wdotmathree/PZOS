#pragma once
#ifndef ARCH_X86_64_PANIC_H
#define ARCH_X86_64_PANIC_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#include <x86_64/isr.h>

// As long as `msg` is a string literal, it should hopefully not touch any registers
#define panic(args...)                            \
	do {                                          \
		struct isr_frame_t *ptr;                  \
		asm volatile("push rax\n\t"               \
					 "push rbx\n\t"               \
					 "push rcx\n\t"               \
					 "push rdx\n\t"               \
					 "push rsi\n\t"               \
					 "push rdi\n\t"               \
					 "push rbp\n\t"               \
					 "push r8\n\t"                \
					 "push r9\n\t"                \
					 "push r10\n\t"               \
					 "push r11\n\t"               \
					 "push r12\n\t"               \
					 "push r13\n\t"               \
					 "push r14\n\t"               \
					 "push r15\n\t"               \
					 "mov %0, rsp" : "=rm"(ptr)); \
		_panic(ptr, args);                        \
		__builtin_unreachable();                  \
	} while (0)

__attribute__((noreturn)) void _panic(struct isr_frame_t *, const char *msg, ...);

__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("hlt");
	}
}

#endif
