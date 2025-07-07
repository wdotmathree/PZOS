#include <kernel/time.h>

#include <stddef.h>

#include <x86_64/intrin.h>
#include <x86_64/isr.h>

#include <kernel/log.h>

void get_time_default(struct timeval_t *tv) {
	tv->secs = 0;
	tv->usecs = 0;
}

void (*_get_time)(struct timeval_t *tv) = get_time_default;

static uint64_t tsc_freq; // Hz
uint64_t tsc_base; // Base TSC value when kernel was loaded
void get_time_tsc(struct timeval_t *tv) {
	uint32_t lo, hi;
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	uint64_t tsc = (((uint64_t)hi << 32) | lo) - tsc_base;
	tv->secs = tsc / tsc_freq;
	tv->usecs = (tsc % tsc_freq) * 1000000 / tsc_freq;
}

static int calibrate_state = 0;
struct isr_frame_t *calibrate_tsc(struct isr_frame_t *const frame) {
	if (calibrate_state == 0) {
		calibrate_state = 1;
		// Note down the current TSC value
		uint32_t lo, hi;
		asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
		tsc_freq = ((uint64_t)hi << 32) | lo;
	} else {
		calibrate_state = 0;
		// Calculate the frequency of the TSC
		uint32_t lo, hi;
		asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
		uint64_t elapsed = (((uint64_t)hi << 32) | lo) - tsc_freq;

		tsc_freq = elapsed * 18.206507364908855; // Approximate frequency of the PIT
		_get_time = get_time_tsc;
		LOG("TSC", "Detected CPU frequency: %llu MHz", tsc_freq / 1000000);

		// Cleanup
		uint8_t mask = inb(0x21);
		mask |= (1 << 0); // Mask the PIT timer interrupt
		outb(0x21, mask);
		unregister_isr(32);
	}
	outb(0x20, 0x20); // Send EOI

	return frame;
}

/// TODO: Add late HPET initialization

static bool tsc_invariant = true;

void time_init(void) {
	// Check if RDTSC is supported
	uint32_t eax = 1, ebx = 0, ecx = 0, edx = 0;
	cpuid(&eax, &ebx, &ecx, &edx);
	if ((edx & (1 << 4)) == 0) {
		// No TSC is available
		LOG(NULL, "RDTSC not supported, using PIC for timekeeping.");
		_get_time = get_time_tsc;
	}

	// Check if TSC is invariant
	eax = 0x80000007;
	cpuid(&eax, &ebx, &ecx, &edx);
	if ((edx & (1 << 8)) == 0) {
		tsc_invariant = false;
	} else {
		eax = 0x15;
		cpuid(&eax, &ebx, &ecx, &edx);
		if ((ebx & ecx) != 0) {
			tsc_freq = ecx * ebx / eax;
		} else {
			tsc_invariant = false;
		}
	}

	if (tsc_invariant) {
		LOG("TSC", "Invariant TSC frequency: %llu MHz", tsc_freq);
		_get_time = get_time_tsc;
	} else {
		LOG("TSC", "TSC is not invariant, using PIT to calibrate");

		// Initialize PIT to mode 2 with lowest frequency
		outb(0x43, 0b00110100);
		outb(0x40, 0);
		outb(0x40, 0);

		// Unmask the PIT timer interrupt (IRQ0)
		register_isr(0x20, calibrate_tsc, 0);
		uint8_t mask = inb(0x21);
		mask &= ~(1 << 0);
		outb(0x21, mask);
		asm volatile("sti\n\thlt\n\thlt\n\tcli");
	}

	/// TODO: Initialize system time and set up timer interrupts
}
