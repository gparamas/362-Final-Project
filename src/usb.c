#include "usb.h"
#include "tusb.h"
#include <stdio.h>

KeyboardState kb_p1 = {0};
KeyboardState kb_p2 = {0};

void usb_init(void) {
    tusb_init();
}

void usb_task(void) {
    tuh_task();
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
    if (len < 3) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }

    kb_p1 = (KeyboardState){0};
    kb_p2 = (KeyboardState){0};

    for (int i = 2; i < 8; i++) {
        uint8_t key = report[i];
        if (key == 0) continue;

        if (key == 0x1A) kb_p1.up    = true;  // W
        if (key == 0x16) kb_p1.down  = true;  // S
        if (key == 0x04) kb_p1.left  = true;  // A
        if (key == 0x07) kb_p1.right = true;  // D

        if (key == 0x52) kb_p2.up    = true;  // Up arrow
        if (key == 0x51) kb_p2.down  = true;  // Down arrow
        if (key == 0x50) kb_p2.left  = true;  // Left arrow
        if (key == 0x4F) kb_p2.right = true;  // Right arrow
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
}
