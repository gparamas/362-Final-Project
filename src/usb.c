#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb.h"

uint32_t tusb_time_millis_api(void) {
    return to_ms_since_boot(get_absolute_time());
}

KeyboardState kb_p1 = {0};
KeyboardState kb_p2 = {0};

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

    printf("P1: %s%s%s%s  |  P2: %s%s%s%s\n",
        kb_p1.up    ? "W" : "-",
        kb_p1.down  ? "S" : "-",
        kb_p1.left  ? "A" : "-",
        kb_p1.right ? "D" : "-",
        kb_p2.up    ? "^" : "-",
        kb_p2.down  ? "v" : "-",
        kb_p2.left  ? "<" : "-",
        kb_p2.right ? ">" : "-"
    );

    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    printf("[usb] HID device mounted: addr=%d instance=%d\n", dev_addr, instance);
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
}
