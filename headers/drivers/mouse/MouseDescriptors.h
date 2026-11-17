/*
 * SPDX-License-Identifier: MIT
 * CPC: a boot-protocol USB mouse (buttons, X, Y, wheel, pan; no report id).
 */
#pragma once

#include <stdint.h>
#include "tusb.h"

typedef struct __attribute__((packed))
{
	uint8_t buttons;   // bit0 left, bit1 right, bit2 middle
	int8_t x;
	int8_t y;
	int8_t wheel;
	int8_t pan;
} MouseReport;

static const uint8_t mouse_string_language[]    = { 0x09, 0x04 };
static const uint8_t mouse_string_manfacturer[] = "CPC";
static const uint8_t mouse_string_product[]     = "CPC Wii aim mouse (GP2040-CE)";
static const uint8_t mouse_string_version[]     = "1.0";

static const uint8_t *mouse_string_descriptors[] __attribute__((unused)) =
{
	mouse_string_language,
	mouse_string_manfacturer,
	mouse_string_product,
	mouse_string_version
};

static const uint8_t mouse_device_descriptor[] =
{
	sizeof(tusb_desc_device_t),	// bLength
	TUSB_DESC_DEVICE,			// bDescriptorType
	0x10, 0x01,					// bcdUSB
	0x00,						// bDeviceClass
	0x00,						// bDeviceSubClass
	0x00,						// bDeviceProtocol
	64,							// bMaxPacketSize0
	0xfe, 0xca,					// idVendor  (0xCAFE, like the keyboard mode)
	0x02, 0x00,					// idProduct (0x0002)
	0x00, 0x01,					// bcdDevice
	0x01,						// iManufacturer
	0x02,						// iProduct
	0x00,						// iSerialNumber
	0x01						// bNumConfigurations
};

enum
{
	ITF_NUM_HID_MOUSE,
	ITF_NUM_TOTAL_MOUSE
};

#define MOUSE_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define MOUSE_EPNUM_HID        0x81

static const uint8_t mouse_report_descriptor[] =
{
	TUD_HID_REPORT_DESC_MOUSE()
};

static const uint8_t mouse_configuration_descriptor[] =
{
	// Config number, interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_MOUSE, 0, MOUSE_CONFIG_TOTAL_LEN, 32, 100),

	// Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval.
	// The interface protocol MOUSE marks it as a boot-protocol mouse (a Maple adapter, a BIOS).
	TUD_HID_DESCRIPTOR(ITF_NUM_HID_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(mouse_report_descriptor), MOUSE_EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};
