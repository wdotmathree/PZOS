#pragma once
#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdarg.h>
#include <stdio.h>

#include <kernel/ansi.h>
#include <kernel/time.h>
#include <kernel/tty.h>

extern int vprintf(const char *, va_list);

static inline void LOG(const char *src, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct timeval_t tv;
	get_time(&tv);
	if (src)
		printf(ANSI_ESCAPE_GREEN "[%5llu.%06llu] " ANSI_ESCAPE_YELLOW "%s" ANSI_ESCAPE_RESET ": ", tv.secs, tv.usecs, src);
	else
		printf(ANSI_ESCAPE_GREEN "[%5llu.%06llu] " ANSI_ESCAPE_RESET, tv.secs, tv.usecs);

	vprintf(fmt, args);

	tty_puts(ANSI_ESCAPE_RESET "\n");

	va_end(args);
}

static inline void LOGn(const char *src, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct timeval_t tv;
	get_time(&tv);
	if (src)
		printf(ANSI_ESCAPE_GREEN "[%5llu.%06llu] " ANSI_ESCAPE_YELLOW "%s" ANSI_ESCAPE_RESET ": ", tv.secs, tv.usecs, src);
	else
		printf(ANSI_ESCAPE_GREEN "[%5llu.%06llu] " ANSI_ESCAPE_RESET, tv.secs, tv.usecs);

	vprintf(fmt, args);

	va_end(args);
}

#endif
