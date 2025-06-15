#pragma once
#ifndef ARCH_X86_64_PANIC_H
#define ARCH_X86_64_PANIC_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#include <kernel/isr.h>

// As long as `msg` is a string literal, it should hopefully not touch any registers
#define panic(msg)          \
	asm volatile("push %0;" \
				 "int 0x81" : : "rN"(msg))

__attribute__((noreturn)) void _panic(const char *, struct isr_frame_t *);

__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("hlt");
	}
}

#endif
