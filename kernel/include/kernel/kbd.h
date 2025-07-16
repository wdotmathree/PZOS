#pragma once
#ifndef KERNEL_KBD_H
#define KERNEL_KBD_H

#include <stdbool.h>
#include <stdint.h>

// clang-format off
enum KeyCode {
	Key_LCtrl = 0x00, Key_LMeta, Key_LAlt, Key_Space, Key_RAlt, Key_RMeta, Key_Menu, Key_RCtrl,
	Key_Left = 0x0e, Key_Down, Key_Right, Key_Num0, Key_NumPeriod, Key_NumEnter,
	Key_LShift = 0x20, Key_Z, Key_X, Key_C, Key_V, Key_B, Key_N, Key_M, Key_Comma, Key_Period, Key_Slash, Key_RShift,
	Key_Up = 0x2f, Key_Num1 = 0x31, Key_Num2, Key_Num3,
	Key_CapsLock = 0x40, Key_A, Key_S, Key_D, Key_F, Key_G, Key_H, Key_J, Key_K, Key_L, Key_Semicolon, Key_Apostrophe, Key_Enter,
	Key_Num4 = 0x51, Key_Num5, Key_Num6, Key_NumPlus,
	Key_Tab = 0x60, Key_Q, Key_W, Key_E, Key_R, Key_T, Key_Y, Key_U, Key_I, Key_O, Key_P, Key_LeftBracket, Key_RightBracket, Key_Backslash,
	Key_Delete = 0x6e, Key_End, Key_PageDown, Key_Num7, Key_Num8, Key_Num9,
	Key_Grave = 0x80, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9, Key_0, Key_Minus, Key_Equals, Key_Backspace,
	Key_Insert = 0x8e, Key_Home, Key_PageUp, Key_NumLock, Key_NumDivide, Key_NumMultiply, Key_NumMinus,
	Key_Escape = 0xa0, Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
	Key_PrintScreen = 0xae, Key_ScrollLock, Key_Pause,
	Key_Media = 0xc1, Key_VolumeDown, Key_VolumeUp, Key_Mute, Key_Stop, Key_Back, Key_PlayPause, Key_Forward, Key_Mail, Key_HomePage, Key_Calculator = 0xcc,
	Key_MyComputer = 0xe0, Key_Favorites, Key_Search, Key_WWWStop = 0xe5, Key_WWWBack, Key_WWWReload, Key_WWWForward,
	Key_Power = 0xee, Key_Sleep, Key_Wake,

	Key_Invalid = 0xff
};
// clang-format on

enum KeyFlags {
	KeyFlag_None = 0,
	KeyFlag_Released = 1 << 0,
	KeyFlag_Control = 1 << 1,
	KeyFlag_Shift = 1 << 2,
	KeyFlag_Alt = 1 << 3,
};

typedef struct {
	uint8_t code;
	enum KeyFlags flags;
	char codepoint;
} keystroke_t;

void kbd_init(void);

keystroke_t kbd_read(void);
bool kbd_isdown(uint8_t key);

#endif
