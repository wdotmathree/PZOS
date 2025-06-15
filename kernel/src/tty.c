#include <kernel/tty.h>

#include <incbin.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <kernel/ansi.h>
#include <kernel/serial.h>

INCBIN(GLYPHS, "glyphs.bin");

#define GLYPH_WIDTH 9
#define GLYPH_HEIGHT 18
#define TAB_WIDTH 8

#define color_pair(fg, bg) ((((uint64_t)(bg)) << 32) | (fg))

static size_t tty_row = 0;
static size_t tty_col = 0;
static uint64_t tty_color;
static size_t tty_width;
static size_t tty_height;

static uint32_t *tty_buf;
static size_t buf_width;
static size_t buf_height;

static void tty_drawcursor(void) {
	// Bottom of current cell
	size_t index = (tty_row * GLYPH_HEIGHT + (int)(0.9 * GLYPH_HEIGHT)) * buf_width + tty_col * GLYPH_WIDTH;
	for (int i = 0; i < GLYPH_WIDTH; i++) {
		tty_buf[index + i] = ANSI_3BIT_COLORS[ANSI_COLOR_WHITE];
		tty_buf[index + buf_width + i] = ANSI_3BIT_COLORS[ANSI_COLOR_WHITE];
	}
}

static void init_ansicolors(void) {
	memcpy(ANSI_8BIT_COLORS, ANSI_4BIT_COLORS, sizeof(ANSI_4BIT_COLORS));
	// 6x6x6 cube
	for (int i = 0; i < 216; i++) {
		uint16_t r = i / 36;
		uint16_t g = (i / 6) % 6;
		uint16_t b = i % 6;

		r = r * 255 / 5;
		g = g * 255 / 5;
		b = b * 255 / 5;

		ANSI_8BIT_COLORS[i + 16] = (r << 16) | (g << 8) | b;
	}
	// Grayscale ramp
	for (int i = 0; i < 24; i++) {
		uint16_t gray = (i + 1) * 255 / 25;
		ANSI_8BIT_COLORS[i + 232] = gray * 0x010101;
	}
}

void tty_init(const struct limine_framebuffer *framebuffer) {
	init_ansicolors();
	tty_buf = framebuffer->address;
	buf_width = framebuffer->width;
	buf_height = framebuffer->height;
	tty_width = framebuffer->width / GLYPH_WIDTH;
	tty_height = framebuffer->height / GLYPH_HEIGHT;
	tty_color = color_pair(ANSI_3BIT_COLORS[ANSI_COLOR_WHITE], ANSI_3BIT_COLORS[ANSI_COLOR_BLACK]);
	tty_drawcursor();
}

void tty_clear(void) {
	memset(tty_buf, 0, buf_width * buf_height * 4);
	tty_row = 0;
	tty_col = 0;
	tty_drawcursor();
}

static uint32_t mix(uint64_t color, uint8_t alpha) {
	uint16_t fg_r = (color >> 16) & 0xff;
	uint16_t fg_g = (color >> 8) & 0xff;
	uint16_t fg_b = (color >> 0) & 0xff;

	uint16_t bg_r = (color >> 48) & 0xff;
	uint16_t bg_g = (color >> 40) & 0xff;
	uint16_t bg_b = (color >> 32) & 0xff;

	// Simple alpha blending
	uint8_t r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
	uint8_t g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
	uint8_t b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;

	return (r << 16) | (g << 8) | b;
}

static void tty_clearcell(size_t x, size_t y) {
	size_t index = y * GLYPH_HEIGHT * buf_width + x * GLYPH_WIDTH;
	for (int i = 0; i < GLYPH_HEIGHT; i++) {
		memset(tty_buf + index + i * buf_width, 0, GLYPH_WIDTH * 4);
	}
}

static void tty_blitchar(char c, size_t x, size_t y) {
	size_t index = y * GLYPH_HEIGHT * buf_width + x * GLYPH_WIDTH;
	for (int y = 0; y < GLYPH_HEIGHT; y++) {
		for (int x = 0; x < GLYPH_WIDTH; x++) {
			size_t glyph_index = ((c - 0x20) * GLYPH_HEIGHT + y) * GLYPH_WIDTH + x;
			uint32_t pixel = mix(tty_color, gGLYPHSData[glyph_index]);
			tty_buf[index + x + y * buf_width] = pixel;
		}
	}
}

static void tty_scroll(void) {
	memmove(tty_buf, tty_buf + GLYPH_HEIGHT * buf_width, (tty_height - 1) * GLYPH_HEIGHT * buf_width * 4);
	memset(tty_buf + ((tty_height - 1) * GLYPH_HEIGHT) * buf_width, 0, GLYPH_HEIGHT * buf_width * 4);
}

/// TODO: Add buffering to parse control sequences spanning multiple print calls
static int parseescape(const char *s) {
	int i = 0;
	if (s[i++] != '[') {
		return 0; // Not an escape sequence
	}
	int args[16] = {};
	int arg_count = 0;
	char cmd;
	while (true) {
		char c = s[i++];
		if (c >= '0' && c <= '9') {
			args[arg_count] = args[arg_count] * 10 + (c - '0');
		} else if (c == ';') {
			args[++arg_count] = 0;
		} else {
			cmd = c;
			arg_count++;
			break;
		}
	}
	if (cmd == 'm') {
		int n;
		for (int j = 0; j < arg_count; j++) {
			switch (args[j]) {
			case 0: // Reset
				tty_color = color_pair(ANSI_3BIT_COLORS[ANSI_COLOR_WHITE], ANSI_3BIT_COLORS[ANSI_COLOR_BLACK]);
				break;
			case 30 ... 37: // Foreground color
				tty_color = color_pair(ANSI_3BIT_COLORS[args[j] - 30], tty_color >> 32);
				break;
			case 90 ... 97: // ...
				tty_color = color_pair(ANSI_4BIT_COLORS[args[j] - 90 + 8], tty_color >> 32);
				break;
			case 38: // Special foreground color
				n = args[++j];
				if (n == 5) { // 8-bit color
					if (args[j + 1] < 0 || args[j + 1] >= 256)
						break;
					tty_color = color_pair(ANSI_8BIT_COLORS[args[++j]], tty_color >> 32);
				} else if (n == 2) { // 24-bit color
					tty_color = color_pair(
						(args[j + 1] << 16) | (args[j + 2] << 8) | args[j + 3],
						tty_color >> 32
					);
					j += 3; // Skip the next three arguments
				} else {
					continue;
				}
				break;
			case 39: // Reset foreground color
				tty_color = color_pair(ANSI_3BIT_COLORS[ANSI_COLOR_WHITE], tty_color >> 32);
				break;
			case 40 ... 47:
				tty_color = color_pair(tty_color & 0xffffffff, ANSI_3BIT_COLORS[args[j] - 40]);
				break;
			case 100 ... 107: // Background color
				tty_color = color_pair(tty_color & 0xffffffff, ANSI_4BIT_COLORS[args[j] - 100 + 8]);
				break;
			case 48: // Special background color
				n = args[++j];
				if (n == 5) { // 8-bit color
					if (args[j + 1] < 0 || args[j + 1] >= 256)
						break;
					tty_color = color_pair(tty_color & 0xffffffff, ANSI_8BIT_COLORS[args[++j]]);
				} else if (n == 2) { // 24-bit color
					tty_color = color_pair(
						tty_color & 0xffffffff,
						(args[j + 1] << 16) | (args[j + 2] << 8) | args[j + 3]
					);
					j += 3; // Skip the next three arguments
				} else {
					continue;
				}
				break;
			case 49: // Reset background color
				tty_color = color_pair(tty_color & 0xffffffff, ANSI_3BIT_COLORS[ANSI_COLOR_BLACK]);
				break;
			default:
				break;
			}
		}
	}
	return i;
}

static int parsecontrol(const char *s) {
	for (int i = 0;; i++) {
		char c = s[i];
		switch (c) {
		case '\0':
			return i + 1;

		case '\n':
			tty_clearcell(tty_col, tty_row);
			if (++tty_row == tty_height) {
				tty_scroll();
				tty_row--;
			}
			tty_col = 0;
			tty_drawcursor();
			break;

		case '\b':
			if (--tty_col > tty_width) {
				if (tty_row == 0) {
					tty_col++;
					break;
				}
				tty_col += tty_width;
				tty_row--;
			}
			break;

		case '\t':
			tty_clearcell(tty_col, tty_row);
			tty_col += TAB_WIDTH - tty_col % TAB_WIDTH;
			if (tty_col >= tty_width) {
				tty_col = 0;
				if (++tty_row == tty_height) {
					tty_scroll();
					tty_row--;
				}
			}
			tty_drawcursor();
			break;

		case '\r':
			tty_clearcell(tty_col, tty_row);
			tty_col = 0;
			tty_drawcursor();
			break;

		case '\x1b':
			return i + 1 + parseescape(s + i + 1);

		default:
			return i;
		}
	}
}

void tty_putchar(char c) {
	serial_write(c);
	char str[2] = {c, 0};
	if (parsecontrol(str)) {
		tty_drawcursor();
		return;
	}

	tty_blitchar(c, tty_col, tty_row);
	if (++tty_col == tty_width) {
		tty_col = 0;
		if (++tty_row == tty_height) {
			tty_scroll();
			tty_row--;
		}
	}
	tty_drawcursor();
}

void tty_write(const char *data, size_t size) {
	for (size_t i = 0; i < size; i++) {
		int off = parsecontrol(data + i);
		for (; off-- > 0; i++)
			serial_write(data[i]);

		if (i < size)
			tty_putchar(data[i]);
	}
}

int tty_puts(const char *data) {
	int len = strlen(data);
	tty_write(data, len);
	return len;
}
