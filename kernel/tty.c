#include <kernel/tty.h>

#include <incbin.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <kernel/ansi.h>

INCBIN(GLYPHS, "glyphs.bin");

#define GLYPH_WIDTH 9
#define GLYPH_HEIGHT 18
#define TAB_WIDTH 8

#define color_pair(fg, bg) ((((uint64_t)bg) << 32) | (fg))

static size_t tty_row = 0;
static size_t tty_col = 0;
static uint64_t tty_color;
static size_t tty_width;
static size_t tty_height;

static uint32_t *tty_buf;
static size_t buf_width;
static size_t buf_height;

void tty_drawcursor(void) {
	size_t index = ((tty_row + 1) * GLYPH_HEIGHT - 1) * buf_width + tty_col * GLYPH_WIDTH;
	for (int i = 0; i < GLYPH_WIDTH; i++) {
		tty_buf[index + i] = ANSI_3BIT_COLORS[ANSI_COLOR_WHITE];
		tty_buf[index - buf_width + i] = ANSI_3BIT_COLORS[ANSI_COLOR_WHITE];
	}
}

void tty_init(const struct limine_framebuffer *framebuffer) {
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

uint32_t mix(uint64_t color, uint8_t alpha) {
	uint32_t fg_r = (color >> 16) & 0xff;
	uint32_t fg_g = (color >> 8) & 0xff;
	uint32_t fg_b = (color >> 0) & 0xff;

	uint32_t bg_r = (color >> 40) & 0xff;
	uint32_t bg_g = (color >> 32) & 0xff;
	uint32_t bg_b = (color >> 24) & 0xff;

	// Simple alpha blending
	uint8_t r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
	uint8_t g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
	uint8_t b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;

	return (r << 16) | (g << 8) | b;
}

void tty_blitchar(char c, size_t x, size_t y) {
	size_t index = y * GLYPH_HEIGHT * buf_width + x * GLYPH_WIDTH;
	// blit glyph onto buffer
	for (int y = 0; y < GLYPH_HEIGHT; y++) {
		for (int x = 0; x < GLYPH_WIDTH; x++) {
			size_t glyph_index = ((c - 0x20) * GLYPH_HEIGHT + y) * GLYPH_WIDTH + x;
			uint32_t pixel = mix(tty_color, gGLYPHSData[glyph_index]);
			tty_buf[index + x + y * buf_width] = pixel;
		}
	}
}

void tty_scroll(void) {
	memmove(tty_buf, tty_buf + GLYPH_HEIGHT * buf_width, (tty_height - 1) * GLYPH_HEIGHT * buf_width * 4);
	memset(tty_buf + ((tty_height - 1) * GLYPH_HEIGHT) * buf_width, 0, GLYPH_HEIGHT * buf_width * 4);
}

int parseescape(const char *s) {
	for (int i = 0;; i++) {
		char c = s[i];
		switch (c) {
		case '\0':
			return i + 1;

		case '\n':
			tty_blitchar(' ', tty_col, tty_row);
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
			tty_blitchar(' ', tty_col, tty_row);
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
			tty_blitchar(' ', tty_col, tty_row);
			tty_col = 0;
			tty_drawcursor();
			break;

		default:
			return i;
		}
	}
}

void tty_putchar(char c) {
	char str[2] = {c, 0};
	if (parseescape(str)) {
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
		size_t off = parseescape(data + i);
		i += off;
		if (i < size)
			tty_putchar(data[i]);
	}
}

int tty_puts(const char *data) {
	int len = strlen(data);
	tty_write(data, len);
	return len;
}
