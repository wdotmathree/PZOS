#pragma once
#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint64_t secs;
	uint64_t usecs;
} timeval_t;

void time_init(void);

/// TODO: Add RTC support, dates and wall time, and time zones :(

extern void (*_get_time)(timeval_t *tv);

static inline void get_time(timeval_t *tv) {
	_get_time(tv);
}

#endif
