#include <stdio.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __is_libk
#include <kernel/tty.h>

#define putchar(c) tty_putchar(c)
#define puts(s) tty_puts(s)
#endif

// TODO: Add support for flags, width, precision, and length modifiers
// Our current implementation is invalid as it zero extends negative values
int vprintf(const char *format, va_list args) {
	int count = 0;
	while (true) {
		if (*format == '\0')
			return count;
		if (*format != '%') {
			count++;
			putchar(*format++);
			continue;
		}

		format++;
		char c = *(format++);
		if (c == 'd' || c == 'i') {
			int64_t value = va_arg(args, int64_t);
			if (value == -__LONG_LONG_MAX__ - 1) {
				count += puts("-9223372036854775808");
				break;
			}
			if (value < 0) {
				value = -value;
				count++;
				putchar('-');
			}
			char buffer[10];
			int i = 0;
			do {
				buffer[i++] = (value % 10) + '0';
				value /= 10;
			} while (value > 0);

			count += i;
			while (i > 0)
				putchar(buffer[--i]);
		} else if (c == 'u') {
			uint64_t value = va_arg(args, uint64_t);
			char buffer[10];
			int i = 0;
			do {
				buffer[i++] = (value % 10) + '0';
				value /= 10;
			} while (value > 0);

			count += i;
			while (i > 0)
				putchar(buffer[--i]);
		} else if (c == 'x' || c == 'X') {
			uint64_t value = va_arg(args, uint64_t);
			char buffer[16];
			int i = 0;
			do {
				int x = value & 0xf;
				if (x < 10) {
					buffer[i++] = x + '0';
				} else {
					buffer[i++] = (c - 'X' + 'A') + (x - 10);
				}
				value >>= 4;
			} while (value > 0);
			count += i;
			while (i > 0)
				putchar(buffer[--i]);
		} else if (c == 'c') {
			char c = (char)va_arg(args, int);
			putchar(c);
			count++;
		} else if (c == 's') {
			const char *str = va_arg(args, const char *);
			count += puts(str);
		} else if (c == 'n') {
			int *ptr = va_arg(args, int *);
			*ptr = count;
		} else if (c == '%') {
			putchar('%');
			count++;
		} else {
			putchar(c);
			count++;
		}
	}
	return count;
}

int printf(const char *format, ...) {
	va_list args;
	va_start(args, format);

	int result = vprintf(format, args);

	va_end(args);
	return result;
}
