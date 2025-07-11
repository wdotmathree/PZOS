#pragma once
#ifndef X86_64_PS2_H
#define X86_64_PS2_H

#include <stdint.h>

#include <x86_64/isr.h>

void ps2_send_ctrl(uint8_t byte);
void ps2_send_ctrl_long(uint8_t first, uint8_t second);
uint8_t ps2_read_status(void);

void ps2_send_data(uint8_t byte);
uint8_t ps2_read_data(void);

void ps2_register_kbd_isr(isr_handler_t isr);

#endif
