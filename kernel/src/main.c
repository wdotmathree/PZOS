#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/kbd.h>
#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/tty.h>

#define HIST_SIZE 64

static char history[HIST_SIZE][1024];
static size_t history_cnt = 0;
static int history_idx = -1;

static char buf[1024];
static size_t buflen = 0;
static size_t bufpos = 0;

static char prompt[] = "\x1b[92mPZOS@PZOS\x1b[0m:\x1b[94m/\x1b[0m$ ";
static size_t promptlen = 13;

static size_t tty_width;
static size_t tty_height;

static void eval(void) {
	if (buflen == 0)
		return;

	if (history_idx != history_cnt - 1)
		strcpy(history[history_cnt], buf);

	buf[buflen] = '\0'; // Null-terminate the string
	char *args = strchr(buf, ' ');
	if (args) {
		*args = 0;
		args++;
	}

	if (strcmp(buf, "clear") == 0) {
		tty_clear();
	} else if (strcmp(buf, "help") == 0) {
		tty_puts("Available commands: clear, help, echo\n");
	} else if (strcmp(buf, "echo") == 0) {
		if (args)
			tty_puts(args);
		tty_putchar('\n');
	} else {
		tty_puts(buf);
		tty_puts(": Command not found\n");
	}
}

static void clearline(void) {
	// Go to end of input
	printf("\x1b[%dC", buflen - bufpos);
	// Clear all rows associated with the input
	tty_puts("\r\x1b[2K");
	for (int i = 0; i < (buflen + promptlen + 1) / tty_width; i++) {
		tty_puts("\x1b[F\x1b[K");
	}
}

__attribute__((noreturn)) void kmain(tty_dim_t tty_dim) {
	tty_width = tty_dim.width;
	tty_height = tty_dim.height;

	tty_putchar('\n');
	tty_puts(prompt);

	while (true) {
		asm("hlt");
		keystroke_t key;
		while ((key = kbd_read()).code != Key_Invalid) {
			if (key.flags & KeyFlag_Released)
				continue;

			if (key.codepoint >= 0x20) { // Printable characters
				tty_putchar(key.codepoint);
				if (bufpos < buflen) {
					memmove(buf + bufpos + 1, buf + bufpos, buflen - bufpos);
					buf[bufpos] = key.codepoint;
					buflen++;
					bufpos++;
					tty_write(buf + bufpos, buflen - bufpos);
					printf("\x1b[%dD", buflen - bufpos);
				} else {
					buf[bufpos++] = key.codepoint;
					buflen++;
				}
				continue;
			}

			switch (key.code) {
			case Key_Enter:
			case Key_NumEnter:
				// Go to end of input before printing
				printf("\x1b[%dC", buflen - bufpos);
				tty_putchar('\n');
				// Store history if not empty and not duplicate of last command
				if (buf[0] && (history_cnt == 0 || history_idx != history_cnt - 1 || strcmp(history[history_cnt - 1], buf) != 0)) {
					if (history_cnt < HIST_SIZE - 1) {
						buf[buflen] = '\0';
						strcpy(history[history_cnt++], buf);
					} else {
						// Shift if full
						memmove(history, history + 1, sizeof(history) - 2 * sizeof(history[0]));
						strcpy(history[HIST_SIZE - 2], buf);
					}
				}
				eval();
				bufpos = 0;
				buflen = 0;
				tty_puts(prompt);
				history_idx = -1;
				history[history_cnt][0] = '\0';
				break;
			case Key_Backspace:
				if (bufpos == 0)
					break;
				memcpy(buf + bufpos - 1, buf + bufpos, buflen - bufpos);
				bufpos--;
				buflen--;
				// Clear and rewrite everything
				clearline();
				tty_puts(prompt);
				tty_write(buf, buflen);
				printf("\x1b[%dD", buflen - bufpos);
				break;
			case Key_Left:
				if (bufpos == 0)
					break;
				bufpos--;
				tty_puts("\x1b[D");
				break;
			case Key_Right:
				if (bufpos == buflen)
					break;
				bufpos++;
				tty_puts("\x1b[C");
				break;
			case Key_Up:
				if (history_cnt == 0)
					break;
				if (history_idx == -1) {
					memcpy(history[history_cnt], buf, buflen); // Temporarily store current input
					history_idx = history_cnt - 1;
				} else if (history_idx > 0)
					history_idx--;
				clearline();
				tty_puts(prompt);
				tty_puts(history[history_idx]);
				buflen = strlen(history[history_idx]);
				bufpos = buflen;
				memcpy(buf, history[history_idx], buflen);
				break;
			case Key_Down:
				if (history_cnt == 0 || history_idx == -1)
					break;
				if (history_idx == history_cnt - 1) {
					clearline();
					tty_puts(prompt);
					tty_puts(history[history_cnt]);
					buflen = strlen(history[history_cnt]);
					bufpos = buflen;
					memcpy(buf, history[history_cnt], buflen);
					history_idx = -1;
				} else {
					history_idx++;
					clearline();
					tty_puts(prompt);
					tty_puts(history[history_idx]);
					buflen = strlen(history[history_idx]);
					bufpos = buflen;
					memcpy(buf, history[history_idx], buflen);
				}
				break;
			}
		}
	}

	while (true)
		hcf();
	panic(NULL);
}
