#pragma once
#ifndef TTY_H
#define TTY_H

#include <limine.h>
#include <stddef.h>

void tty_init(const struct limine_framebuffer *framebuffer);
void tty_clear(void);
void tty_putchar(char c);
void tty_write(const char *data, size_t size);
void tty_puts(const char *data);

#endif
