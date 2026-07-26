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

// Return the number of microseconds since kernel entry
extern uint64_t (*get_utime)(void);

// Store the time since kernel entry in `tv`
void get_time(timeval_t *tv);

#endif
