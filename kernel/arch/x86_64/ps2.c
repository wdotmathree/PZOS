#include <x86_64/ps2.h>

#include <x86_64/intrin.h>
#include <x86_64/pic.h>

isr_handler_t kbd_handler = NULL;

isr_frame_t *kbd_isr_stub(isr_frame_t *const frame) {
	PIC_eoi(1);

	if (kbd_handler != NULL)
		return kbd_handler(frame);
	else
		return frame;
}

void ps2_send_ctrl(uint8_t byte) {
	if (ps2_read_status() & 0x02)
		io_wait();
	outb(0x64, byte);
	io_wait();
}

void ps2_send_data(uint8_t byte) {
	outb(0x60, byte);
	io_wait();
}

void ps2_send_ctrl_long(uint8_t first, uint8_t second) {
	ps2_send_ctrl(first);

	if (ps2_read_status() & 0x02)
		io_wait();
	outb(0x60, second);
	io_wait();
}

uint8_t ps2_read_status(void) {
	return inb(0x64);
}

uint8_t ps2_read_data(void) {
	if (ps2_read_status() & 0x01)
		return inb(0x60);

	return 0;
}

void ps2_register_kbd_isr(isr_handler_t isr) {
	register_isr(0x21, kbd_isr_stub, 0);
	kbd_handler = isr;

	// Enable keyboard interrupt
	ps2_send_ctrl(0x20);
	uint8_t config = ps2_read_data();
	ps2_send_ctrl_long(0x60, config | 0x01);

	// Unmask in PIC
	PIC_unmask(1);
}
