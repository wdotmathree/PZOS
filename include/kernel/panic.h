#pragma once
#ifndef PANIC_H
#define PANIC_H

__attribute__((noreturn)) void panic(const char *msg);
__attribute__((noreturn)) static inline void halt(void) {
	while (1) {
		asm("cli;hlt");
	}
}

#endif
