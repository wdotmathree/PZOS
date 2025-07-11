#pragma once
#ifndef KERNEL_PS2_H
#define KERNEL_PS2_H

#include_next <ps2.h>

#include <stdbool.h>

bool ps2_init(void);

bool ps2_send_kbd(uint8_t byte);
bool ps2_send_mse(uint8_t byte);

int ps2_read_sync(void);

#endif
