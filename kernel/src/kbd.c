#include <kernel/kbd.h>

#include <kernel/log.h>
#include <kernel/ps2.h>

static char rbuf[256];
static uint8_t rbuf_ridx = 0;
static uint8_t rbuf_widx = 0;

static char wbuf[256];
static uint8_t wbuf_ridx = 0;
static uint8_t wbuf_widx = 0;
static uint8_t retry_cnt = 0;

static bool active = false;

static struct isr_frame_t *kbd_isr(struct isr_frame_t *const frame) {
	uint8_t scancode = ps2_read_data();

	LOG("KBD", "Scancode: 0x%x", scancode);
	if (scancode == 0xfa) {
		if (++wbuf_ridx != wbuf_widx)
			ps2_send_kbd(wbuf[wbuf_ridx]);
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
		return frame;
	}
	if (!active)
		return frame;

	// Begin parsing scancode sequences

	return frame;
}

static int kbd_read(void) {
	if (rbuf_widx == rbuf_ridx)
		return -1;
	return rbuf[rbuf_ridx++];
}

static void kbd_write(char c) {
	wbuf[wbuf_widx] = c;
	if (wbuf_widx++ == wbuf_ridx)
		ps2_send_kbd(c);
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
