// CPC mouse mode (SPDX-License-Identifier: MIT).
//
// The UartInput addon fills the gamepad state from the Pi's UART frame; this driver
// turns that state into a USB boot-protocol mouse:
//   buttons  B1 = left click, B2 = right click, B3 = middle click
//   wheel    d-pad UP = one notch up, d-pad DOWN = one notch down (on the press)
//   motion   the left stick is the AIM (a POSITION, 0..255 across / down the picture, 0x80
//            = centre). A mouse reports MOVEMENT, so every call the CHANGE in the aim is
//            turned into counts (MOUSE_COUNTS_X256 per aim unit). A host that clips its
//            cursor at the screen edge re-syncs when you sweep past an edge. A jump of
//            more than MOUSE_UNITS_JUMP in one step (the first one, or the aim returning
//            after a neutral frame) only re-anchors: it moves nothing.
#include "drivers/mouse/MouseDriver.h"
#include "drivers/shared/driverhelper.h"
#include "drivers/hid/HIDDescriptors.h"
#include "pico/time.h"

#include <stdlib.h>
#include <string.h>

#define MOUSE_COUNTS_X256     768     // mouse counts per aim unit, x256: 768 = 3 counts per unit
#define MOUSE_UNITS_JUMP      64
#define MOUSE_REPORT_PERIOD_MS 8      // USB reports at most every 8 ms (125 Hz)

void MouseDriver::initialize() {
	mouseReport = { };

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "MOUSE",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

// Whole counts out of an x256 accumulator, at most one report's worth (int8).
static int8_t takeCounts(int32_t &acc) {
	int32_t v = acc / 256;
	if (v > 127) v = 127;
	if (v < -127) v = -127;
	acc -= v * 256;
	return (int8_t)v;
}

bool MouseDriver::process(Gamepad * gamepad) {
	uint8_t buttons = (uint8_t)((gamepad->pressedB1() ? 0x01 : 0) |
	                            (gamepad->pressedB2() ? 0x02 : 0) |
	                            (gamepad->pressedB3() ? 0x04 : 0));

	bool up = gamepad->pressedUp(), down = gamepad->pressedDown();
	if (up && !lastUp && wheel < 127) wheel++;
	if (down && !lastDown && wheel > -127) wheel--;
	lastUp = up;
	lastDown = down;

	int ax = gamepad->state.lx >> 8;
	int ay = gamepad->state.ly >> 8;
	if (!anchored || abs(ax - prevX) > MOUSE_UNITS_JUMP || abs(ay - prevY) > MOUSE_UNITS_JUMP) {
		anchored = true;                       // first call, or the aim jumped: just re-anchor
	} else {
		accumX += (int32_t)(ax - prevX) * MOUSE_COUNTS_X256;
		accumY += (int32_t)(ay - prevY) * MOUSE_COUNTS_X256;
	}
	prevX = ax;
	prevY = ay;

	uint32_t now = to_ms_since_boot(get_absolute_time());
	if (now - lastReportMs < MOUSE_REPORT_PERIOD_MS) return false;

	// Wake up TinyUSB device
	if (tud_suspended())
		tud_remote_wakeup();

	if (!tud_hid_ready()) return false;
	lastReportMs = now;

	int8_t dx = takeCounts(accumX);
	int8_t dy = takeCounts(accumY);
	if (dx || dy || wheel || buttons != sentButtons) {
		mouseReport.buttons = buttons;
		mouseReport.x = dx;
		mouseReport.y = dy;
		mouseReport.wheel = wheel;
		mouseReport.pan = 0;
		if (tud_hid_report(0, &mouseReport, sizeof(mouseReport))) {   // report id 0: no id byte
			sentButtons = buttons;
			wheel = 0;
			return true;
		}
	}
	return false;
}

// tud_hid_get_report_cb
uint16_t MouseDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	memcpy(buffer, &mouseReport, sizeof(mouseReport));
	return sizeof(mouseReport);
}

// A mouse has no host-to-device reports
void MouseDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool MouseDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
	return false;
}

const uint16_t * MouseDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
	const char *value = (const char *)mouse_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * MouseDriver::get_descriptor_device_cb() {
	return mouse_device_descriptor;
}

const uint8_t * MouseDriver::get_hid_descriptor_report_cb(uint8_t itf) {
	return mouse_report_descriptor;
}

const uint8_t * MouseDriver::get_descriptor_configuration_cb(uint8_t index) {
	return mouse_configuration_descriptor;
}

const uint8_t * MouseDriver::get_descriptor_device_qualifier_cb() {
	return nullptr;
}

uint16_t MouseDriver::GetJoystickMidValue() {
	return HID_JOYSTICK_MID << 8;
}
