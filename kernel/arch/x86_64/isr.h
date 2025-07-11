#pragma once
#ifndef ARCH_X86_64_ISR_H
#define ARCH_X86_64_ISR_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#include <stdint.h>

#define IDT_SIZE 256

typedef struct {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t irq_num;
	uint64_t error_code;

	uint64_t isr_rip;
	uint64_t isr_cs;
	uint64_t isr_rflags;
	uint64_t isr_rsp;
	uint64_t isr_ss;
} isr_frame_t;

#define KERNEL_CS (1 << 3)
#define KERNEL_DS (2 << 3)
#define TSS (5 << 3) // For the future

typedef isr_frame_t *(*isr_handler_t)(isr_frame_t *const);

void isr_init(void);

void register_isr(uint8_t num, isr_handler_t handler, uint8_t dpl);
void unregister_isr(uint8_t num);

#endif
