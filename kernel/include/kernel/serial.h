#pragma once
#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

int serial_init(void);
char serial_read(void);
void serial_write(char c);

#endif
