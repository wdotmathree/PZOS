#pragma once
#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <kernel/isr.h>

__attribute__((noreturn)) void panic(const char *msg);
__attribute__((noreturn)) void ipanic(const char *msg, struct isr_frame_t *frame);
__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("cli;hlt");
	}
}

#endif
