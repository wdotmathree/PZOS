#include <kernel/kbd.h>

#include <kernel/log.h>
#include <kernel/ps2.h>
#include <kernel/spinlock.h>

// clang-format off
static uint8_t normal_map[] = {
	0xff,0xa9,0xff,0xa5,0xa3,0xa1,0xa2,0xac,0xff,0xaa,0xa8,0xa6,0xa4,0x60,0x80,0xff,
	0xff,0x02,0x20,0xff,0x00,0x61,0x81,0xff,0xff,0xff,0x21,0x42,0x41,0x62,0x82,0xff,
	0xff,0x23,0x22,0x43,0x63,0x84,0x83,0xff,0xff,0x03,0x24,0x44,0x65,0x64,0x85,0xff,
	0xff,0x26,0x25,0x46,0x45,0x66,0x86,0xff,0xff,0xff,0x27,0x47,0x67,0x87,0x88,0xff,
	0xff,0x28,0x48,0x68,0x69,0x8a,0x89,0xff,0xff,0x29,0x2a,0x49,0x4a,0x6a,0x8b,0xff,
	0xff,0xff,0x4b,0xff,0x6b,0x8c,0xff,0xff,0x40,0x2b,0x4c,0x6c,0xff,0x6d,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0x8d,0xff,0xff,0x31,0xff,0x51,0x71,0xff,0xff,0xff,
	0x11,0x13,0x32,0x52,0x53,0x72,0xa0,0x91,0xab,0x54,0x33,0x94,0x93,0x73,0xaf,0xff,
	0xff,0xff,0xff,0xa7
};

static uint8_t extended_map[] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xe2,0x04,0xae,0xff,0x07,0xc6,0xff,0xff,0xe1,0xff,0xff,0xff,0xff,0xff,0xff,0x01,
	0xe7,0xc2,0xff,0xc4,0xff,0xff,0xff,0x05,0xe5,0xff,0xff,0xcc,0xff,0xff,0xff,0x06,
	0xe6,0xff,0xc3,0xff,0xc7,0xff,0xff,0xee,0xe8,0xff,0xca,0xc5,0xff,0xff,0xff,0xef,
	0xe0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc9,0xff,0x92,0xff,0xff,0xc8,0xff,0xff,
	0xc1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x14,0xff,0xff,0xff,0xf1,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x6f,0xff,0x0e,0x8f,0xff,0xff,0xff,
	0x8e,0x6e,0x0f,0xff,0x10,0x2f,0xff,0xb0,0xff,0xff,0x70,0xff,0xae,0x90,
};

static char codepoint_map[] = {
	0, 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, '0', 0, '.', '\n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, 0,
	0, 0, '1', '2', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\n', 0, 0, 0, 0,
	'4', '5', '6', '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\',
	0, 0, 0, '7', '8', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	0, 0, 0, 0, '/', '*', '-'
};

static char shifted_map[] = {
	0, 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, '\n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '\n', 0, 0, 0, 0,
	0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|',
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
	0, 0, 0, 0, '/', '*', '-'
};
// clang-format on

static keystroke_t rbuf[256];
static uint8_t rbuf_ridx = 0;
static uint8_t rbuf_widx = 0;

static char wbuf[256];
static uint8_t wbuf_ridx = 0;
static uint8_t wbuf_widx = 0;
static uint8_t retry_cnt = 0;

static bool active = false;

static bool keystates[256] = {false};

typedef enum {
	STATE_NORMAL = 0,
	STATE_EXTENDED = 1 << 0,
	STATE_BREAK = 1 << 1,
	STATE_DIGRAPH = 1 << 2,
	STATE_TWOBYTE = 1 << 3,
} state_t;
static state_t current_state = STATE_NORMAL;
static uint16_t prev = 0xffff;

static spinlock_t kbd_lock = SPINLOCK_INITIALIZER;

static isr_frame_t *kbd_isr(isr_frame_t *const frame) {
	uint8_t scancode = ps2_read_data();
	uint64_t prev_irq = spin_acquire_irqsave(&kbd_lock);

	if (scancode == 0xfa) {
		if (++wbuf_ridx != wbuf_widx)
			ps2_send_kbd(wbuf[wbuf_ridx]);
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	} else if (scancode == 0xfe) {
		if (retry_cnt < 3) {
			retry_cnt++;
			LOG("KBD", "Resending last byte, attempt %d", retry_cnt);
			ps2_send_kbd(wbuf[wbuf_ridx]);
		} else {
			LOG("KBD", "Failed to send last byte after 3 attempts");
			retry_cnt = 0;
		}
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	}
	if (!active) {
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	}

	// Begin parsing scancode sequences
	if (scancode == 0xe0) {
		current_state |= STATE_EXTENDED;
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	} else if (scancode == 0xf0) {
		current_state |= STATE_BREAK;
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	} else if (scancode == 0xe1) {
		current_state |= STATE_TWOBYTE;
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	} else {
		uint8_t key = 0xff;

		if ((current_state & STATE_EXTENDED) && (scancode == 0x12 || scancode == 0x7c) && !(current_state & STATE_DIGRAPH)) {
			current_state |= STATE_DIGRAPH;
			spin_release_irqrestore(&kbd_lock, prev_irq);
			return frame;
		} else if (current_state & STATE_TWOBYTE) {
			if (prev == 0xffff) {
				prev = scancode;
				spin_release_irqrestore(&kbd_lock, prev_irq);
				return frame;
			}

			prev = 0xffff;
			// Currently we can just ignore the top byte
			key = extended_map[scancode];
		} else if (current_state & STATE_EXTENDED) {
			if (scancode >= sizeof(extended_map))
				key = 0xff;
			else
				key = extended_map[scancode];
		} else {
			if (scancode >= sizeof(normal_map))
				key = 0xff;
			else
				key = normal_map[scancode];
		}

		enum KeyFlags flags = KeyFlag_None;
		if (current_state & STATE_BREAK) {
			keystates[key] = false;
			flags |= KeyFlag_Released;
		} else {
			keystates[key] = true;
		}
		if (keystates[Key_LCtrl] || keystates[Key_RCtrl])
			flags |= KeyFlag_Control;
		if (keystates[Key_LShift] || keystates[Key_RShift])
			flags |= KeyFlag_Shift;
		if (keystates[Key_LAlt] || keystates[Key_RAlt])
			flags |= KeyFlag_Alt;

		char codepoint = '\0';
		if (flags & KeyFlag_Shift) {
			if (key < sizeof(shifted_map) && shifted_map[key] != 0)
				codepoint = shifted_map[key];
		} else {
			if (key < sizeof(codepoint_map) && codepoint_map[key] != 0)
				codepoint = codepoint_map[key];
		}

		rbuf[rbuf_widx++] = (keystroke_t){
			.code = key,
			.flags = flags,
			.codepoint = codepoint,
		};

		current_state = STATE_NORMAL;
		spin_release_irqrestore(&kbd_lock, prev_irq);
		return frame;
	}
}

keystroke_t kbd_read(void) {
	uint64_t flags = spin_acquire_irqsave(&kbd_lock);
	if (rbuf_widx == rbuf_ridx) {
		spin_release_irqrestore(&kbd_lock, flags);
		return (keystroke_t){0xff};
	}
	keystroke_t k = rbuf[rbuf_ridx++];
	spin_release_irqrestore(&kbd_lock, flags);
	return k;
}

bool kbd_is_down(uint8_t key) {
	uint64_t flags = spin_acquire_irqsave(&kbd_lock);
	bool v = keystates[key];
	spin_release_irqrestore(&kbd_lock, flags);
	return v;
}

static void kbd_write(char c) {
	uint64_t flags = spin_acquire_irqsave(&kbd_lock);
	wbuf[wbuf_widx] = c;
	if (wbuf_widx++ == wbuf_ridx)
		ps2_send_kbd(c);
	spin_release_irqrestore(&kbd_lock, flags);
}

void kbd_init(void) {
	if (!ps2_init())
		return;

	ps2_register_kbd_isr(kbd_isr);

	kbd_write(0xf0);
	kbd_write(0x02);

	kbd_write(0xf4);
	active = true;
}
