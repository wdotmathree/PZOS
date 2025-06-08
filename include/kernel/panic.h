#pragma once
#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <kernel/isr.h>

// As long as `msg` is a string literal, it should hopefully not touch any registers
#define panic(msg)          \
	asm volatile("push %0;" \
				 "int 0x81" : : "g"(msg))

__attribute__((noreturn)) void _panic(const char *, struct isr_frame_t *);

__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("cli;hlt");
	}
}

#endif
