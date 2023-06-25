#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __is_libk
#include <tty.h>
#endif

#ifdef __is_libk

int printf(const char *format, ...) {
	int written = 0;
	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format != '%') {
			terminal_putchar(*(format++));
			written++;
			continue;
		}
		if (*(++format) == '%') {
			terminal_putchar('%');
			format++;
			written++;
			continue;
		}
		if (*format == 'c') {
			terminal_putchar(va_arg(args, int));
			format++;
			written++;
			continue;
		}
		if (*format == 's') {
			const char *str = va_arg(args, const char *);
			while (*str) {
				terminal_putchar(*(str++));
				written++;
			}
			format++;
			continue;
		}
		if (*format == 'd') {
			int num = va_arg(args, int);
			if (num < 0) {
				terminal_putchar('-');
				written++;
				num = -num;
			}
			int tmp = 0;
			if (!num) {
				terminal_putchar('0');
				written++;
				continue;
			}
			while (num) {
				tmp *= 10;
				tmp += num % 10;
				num /= 10;
			}
			while (tmp) {
				terminal_putchar(tmp % 10 + '0');
				written++;
				tmp /= 10;
			}
			format++;
			continue;
		}
	}

	va_end(args);
	return written;
}

#else
#endif
