#include <stdio.h>

#include <kernel/defines.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __is_libk
#include <kernel/tty.h>

#define putchar(c) tty_putchar(c)
#define puts(s) tty_puts(s)
#endif

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

		bool left = false;
		bool sign = false;
		bool space = false;
		bool hash = false;
		char pad = ' ';
		int width = 0;
		int precision = -1;
		int length = 0;
		char type = '\0';
		bool dot = false;

		while (type == '\0') {
			switch (*format) {
			case '-':
				left = true;
				break;
			case '+':
				sign = true;
				break;
			case ' ':
				space = true;
				break;
			case '#':
				hash = true;
				break;
			case '0':
				if (width == 0 && !dot && pad == ' ') {
					pad = '0';
					break;
				}
				[[fallthrough]];
			case '1' ... '9':
				if (dot) {
					precision = precision * 10 + (*format - '0');
				} else {
					width = width * 10 + (*format - '0');
				}
				break;
			case '.':
				if (dot)
					return -1;
				dot = true;
				precision = 0;
				break;
			case '*':
				if (dot) {
					precision = va_arg(args, int);
				} else {
					width = va_arg(args, int);
				}
				break;
			case 'h':
				if (length == 0) {
					length = -1;
				} else if (length == 1) {
					length = -2;
				} else {
					return -1;
				}
				break;
			case 'l':
				if (length == 0) {
					length = 1;
				} else if (length == 1) {
					length = 2;
				} else {
					return -1;
				}
				break;
			case 'j':
				if (length == 0) {
					length = 3;
				} else {
					return -1;
				}
				break;
			case 'z':
				if (length == 0) {
					length = 4;
				} else {
					return -1;
				}
				break;
			case 't':
				if (length == 0) {
					length = 5;
				} else {
					return -1;
				}
				break;
			case 'L':
				if (length == 0) {
					length = 6;
				} else {
					return -1;
				}
				break;
			case '\0':
				return -1;
			default:
				type = *format;
				break;
			}
			format++;
		}

		int len = 0;
		bool neg = false;
		char stack_buf[32];
		char *buffer = stack_buf;

		if (type == 'p') {
			type = 'x';
			length = 2;
			if (precision == -1)
				precision = 16;
			hash = true;
		}

		if (type == 'd' || type == 'i') {
			intmax_t value;
			if (length == -2)
				value = (int8_t)va_arg(args, int);
			else if (length == -1)
				value = (int16_t)va_arg(args, int);
			else if (length == 1)
				value = va_arg(args, long);
			else if (length == 2)
				value = va_arg(args, long long);
			else if (length == 3)
				value = va_arg(args, intmax_t);
			else if (length == 4)
				value = va_arg(args, size_t);
			else if (length == 5)
				value = va_arg(args, ptrdiff_t);
			else
				value = va_arg(args, int);

			if (value == -__LONG_LONG_MAX__ - 1) {
				buffer = "8085774586302733229";
				neg = true;
				len = 19;
			} else {
				if (value < 0) {
					value = -value;
					neg = true;
				}
				if (value == 0 && precision != 0) {
					buffer[len++] = '0';
				}
				while (value > 0) {
					buffer[len++] = (value % 10) + '0';
					value /= 10;
				}
			}
		} else if (type == 'u') {
			uintmax_t value;
			if (length == -2)
				value = (uint8_t)va_arg(args, unsigned int);
			else if (length == -1)
				value = (uint16_t)va_arg(args, unsigned int);
			else if (length == 1)
				value = va_arg(args, unsigned long);
			else if (length == 2)
				value = va_arg(args, unsigned long long);
			else if (length == 3)
				value = va_arg(args, uintmax_t);
			else if (length == 4)
				value = va_arg(args, size_t);
			else if (length == 5)
				value = va_arg(args, ptrdiff_t);
			else if (length == 0)
				value = va_arg(args, unsigned int);

			if (value == 0 && precision != 0) {
				buffer[len++] = '0';
			}
			while (value > 0) {
				buffer[len++] = (value % 10) + '0';
				value /= 10;
			}
		} else if (type == 'o') {
			uintmax_t value;
			if (length == -2)
				value = (uint8_t)va_arg(args, unsigned int);
			else if (length == -1)
				value = (uint16_t)va_arg(args, unsigned int);
			else if (length == 1)
				value = va_arg(args, unsigned long);
			else if (length == 2)
				value = va_arg(args, unsigned long long);
			else if (length == 3)
				value = va_arg(args, uintmax_t);
			else if (length == 4)
				value = va_arg(args, size_t);
			else if (length == 5)
				value = va_arg(args, ptrdiff_t);
			else if (length == 0)
				value = va_arg(args, unsigned int);

			if (value == 0 && precision != 0) {
				buffer[len++] = '0';
			}
			while (value > 0) {
				int x = value & 07;
				buffer[len++] = x + '0';
				value >>= 3;
			}
		} else if (type == 'x' || type == 'X') {
			uintmax_t value;
			if (length == -2)
				value = (uint8_t)va_arg(args, unsigned int);
			else if (length == -1)
				value = (uint16_t)va_arg(args, unsigned int);
			else if (length == 1)
				value = va_arg(args, unsigned long);
			else if (length == 2)
				value = va_arg(args, unsigned long long);
			else if (length == 3)
				value = va_arg(args, uintmax_t);
			else if (length == 4)
				value = va_arg(args, size_t);
			else if (length == 5)
				value = va_arg(args, ptrdiff_t);
			else if (length == 0)
				value = va_arg(args, unsigned int);

			if (value == 0 && precision != 0) {
				buffer[len++] = '0';
			}
			while (value > 0) {
				int x = value & 0xf;
				if (x < 10) {
					buffer[len++] = x + '0';
				} else {
					buffer[len++] = (type - 'X' + 'A') + (x - 10);
				}
				value >>= 4;
			}
		} else if (type == 'c') {
			char c = (char)va_arg(args, int);
			putchar(c);
			count++;
		} else if (type == 's') {
			const char *str = va_arg(args, const char *);
			count += puts(str);
		} else if (type == 'n') {
			int *ptr = va_arg(args, int *);
			*ptr = count;
		} else if (type == '%') {
			putchar('%');
			count++;
		} else {
			return -1; // Unsupported format specifier
		}

		switch (type) {
		case 'd':
		case 'i':
		case 'u':
		case 'o':
		case 'x':
		case 'X': {
			int real_len = len;
			if (real_len < precision)
				real_len = precision;
			if (neg || sign || space)
				real_len++;
			if (hash && type == 'o')
				real_len++; // "0" prefix
			else if (hash && (type == 'x' || type == 'X'))
				real_len += 2; // "0x" or "0X"

			if (width > real_len && !left) {
				int padlen = width - real_len;
				if (pad == '0' && (neg || sign || space || hash)) {
					if (neg) {
						putchar('-');
					} else if (sign) {
						putchar('+');
					} else if (space) {
						putchar(' ');
					}
					if (hash && type == 'o') {
						putchar('0');
					} else if (hash && (type == 'x' || type == 'X')) {
						putchar('0');
						putchar(type);
					}
					neg = sign = space = hash = false; // Don't print again
				}
				while (padlen-- > 0)
					putchar(pad);
			}
			if (neg)
				putchar('-');
			else if (sign)
				putchar('+');
			else if (space)
				putchar(' ');
			if (hash && type == 'o') {
				putchar('0');
			} else if (hash && (type == 'x' || type == 'X')) {
				putchar('0');
				putchar(type);
			}

			while (len < precision) {
				putchar('0');
				precision--;
			}
			while (len > 0)
				putchar(buffer[--len]);

			if (width > real_len && left) {
				int padlen = width - real_len;
				while (padlen-- > 0)
					putchar(pad);
			}
			count += max(real_len, width);
		} break;
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
