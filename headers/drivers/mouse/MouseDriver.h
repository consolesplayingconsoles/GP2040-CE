/*
 * SPDX-License-Identifier: MIT
 * CPC mouse mode: the gamepad state (fed by the UartInput addon) becomes a USB mouse.
 */

#ifndef _MOUSE_DRIVER_H_
#define _MOUSE_DRIVER_H_

#include "gpdriver.h"
#include "drivers/mouse/MouseDescriptors.h"

class MouseDriver : public GPDriver {
public:
    virtual void initialize();
    virtual bool process(Gamepad * gamepad);
    virtual void initializeAux() {}
    virtual void processAux() {}
    virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
    virtual bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
    virtual const uint16_t * get_descriptor_string_cb(uint8_t index, uint16_t langid);
    virtual const uint8_t * get_descriptor_device_cb();
    virtual const uint8_t * get_hid_descriptor_report_cb(uint8_t itf);
    virtual const uint8_t * get_descriptor_configuration_cb(uint8_t index);
    virtual const uint8_t * get_descriptor_device_qualifier_cb();
    virtual uint16_t GetJoystickMidValue();
    virtual USBListener * get_usb_auth_listener() { return nullptr; }
private:
    MouseReport mouseReport = { };
    int32_t accumX = 0, accumY = 0;      // movement not yet sent, in 1/256 counts
    int8_t  wheel = 0;
    uint8_t sentButtons = 0;
    bool    lastUp = false, lastDown = false;
    bool    anchored = false;
    int     prevX = 0x80, prevY = 0x80;
    uint32_t lastReportMs = 0;
};

#endif // _MOUSE_DRIVER_H_
