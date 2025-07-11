#include <kernel/ps2.h>

#include <kernel/acpi.h>
#include <kernel/log.h>
#include <kernel/time.h>

static inline uint8_t read_config(void) {
	ps2_send_ctrl(0x20);
	return ps2_read_data();
}

bool ps2_init(void) {
	ACPI_FADT *f = acpi_get_table(ACPI_SIG("FACP"));
	if (f != NULL && (f->BootArchitectureFlags & 0x02) == 0) {
		LOG("PS2", "No PS2 controller found, skipping initialization");
		return false;
	}

	// Disable both PS/2 ports
	ps2_send_ctrl(0xad);
	ps2_send_ctrl(0xa7);

	// Clear any data in the buffer
	ps2_read_data();

	// Disable interrupts and translation, enable first port
	ps2_send_ctrl(0x20);
	uint8_t config = ps2_read_sync();
	ps2_send_ctrl_long(0x60, config & ~0x43);
	ps2_send_ctrl(0xae);

	// Reset devices
	ps2_send_kbd(0xff);
	int first = ps2_read_sync();
	int second = ps2_read_sync();
	if ((first == 0xfa && second == 0xaa) || (first == 0xaa && second == 0xfa)) {
		LOG("PS2", "Keyboard self-test successful");
	} else {
		LOG("PS2", "Keyboard self-test failed");
		return false;
	}

	// Register default ISRs
	ps2_register_kbd_isr(NULL);

	return true;
}

bool ps2_send_kbd(uint8_t byte) {
	for (int i = 0; i < 10000; i++) {
		if (!(ps2_read_status() & 0x02)) {
			ps2_send_data(byte);
			return true;
		}
	}
	return false;
}

bool ps2_send_mse(uint8_t byte) {
	ps2_send_ctrl(0xd4);
	for (int i = 0; i < 1000; i++) {
		if (!(ps2_read_status() & 0x04)) {
			ps2_send_ctrl_long(0xd1, byte);
			return true;
		}
	}
	return false;
}

int ps2_read_sync(void) {
	for (int i = 0; i < 1000; i++) {
		if (ps2_read_status() & 0x01) {
			return ps2_read_data();
		}
	}
	return -1;
}
