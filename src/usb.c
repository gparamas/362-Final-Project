#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb.h"

// These are the shared keyboard states that the game reads every frame.
// They get updated by the HID report callback below.
volatile KeyboardState kb_p1 = {0};  // WASD
volatile KeyboardState kb_p2 = {0};  // Arrow keys

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
    if (len < 3) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }

    KeyboardState p1 = {0};
    KeyboardState p2 = {0};

    // Some keyboards prepend a 1-byte report ID before the boot-protocol
    // payload. Skip it when the length is odd (9 bytes instead of 8).
    int start = (len >= 9) ? 3 : 2;

    for (int i = start; i < len && i < start + 6; i++) {
        uint8_t key = report[i];
        if (key == 0) continue;

        // WASD
        if (key == 0x1A) p1.up    = true;  // W
        if (key == 0x16) p1.down  = true;  // S
        if (key == 0x04) p1.left  = true;  // A
        if (key == 0x07) p1.right = true;  // D

        // Arrow keys
        if (key == 0x52) p2.up    = true;
        if (key == 0x51) p2.down  = true;
        if (key == 0x50) p2.left  = true;
        if (key == 0x4F) p2.right = true;
    }

    kb_p1 = p1;
    kb_p2 = p2;

    printf("P1: %s%s%s%s  |  P2: %s%s%s%s\n",
        p1.up    ? "W" : "-",
        p1.down  ? "S" : "-",
        p1.left  ? "A" : "-",
        p1.right ? "D" : "-",
        p2.up    ? "^" : "-",
        p2.down  ? "v" : "-",
        p2.left  ? "<" : "-",
        p2.right ? ">" : "-"
    );

    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    printf("[usb] HID mounted dev=%u instance=%u\n", dev_addr, instance);
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    // Clear input if the keyboard is unplugged so players don't keep moving.
    KeyboardState zero = {0};
    kb_p1 = zero;
    kb_p2 = zero;
}
