#pragma once
#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

#include <limine.h>
#include <stddef.h>

void tty_init(const struct limine_framebuffer *framebuffer);
void tty_clear(void);
void tty_putchar(char c);
void tty_write(const char *data, size_t size);
int tty_puts(const char *data);

#endif
