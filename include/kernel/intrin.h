#pragma once
#ifndef KERNEL_INTRIN_H
#define KERNEL_INTRIN_H

#include <stddef.h>
#include <stdint.h>

static inline void nop(void) {
	asm("nop");
}

static inline void outb(uint16_t port, uint8_t value) {
	asm("out %0, %1" : : "Nd"(port), "a"(value));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm("in %0, %1" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void io_wait(void) {
	asm("out 0x80, al");
}

static inline void lidt(void *ptr, size_t size) {
	struct {
		uint16_t size;
		void *addr;
	} __attribute__((packed)) idtr = {size - 1, ptr};
	asm("lidt %0" : : "m"(idtr));
}

static inline uint64_t rdmsr(uint32_t msr) {
	uint32_t low, high;
	asm("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

#endif
