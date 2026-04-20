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

volatile bool g_usb_device_mounted = false;
volatile bool g_usb_hid_ready = false;

void tuh_mount_cb(uint8_t dev_addr) {
    (void)dev_addr;
    g_usb_device_mounted = true;
}

void tuh_umount_cb(uint8_t dev_addr) {
    (void)dev_addr;
    g_usb_device_mounted = false;
    g_usb_hid_ready = false;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
    (void)dev_addr;
    (void)instance;

    /* Boot keyboard: 8 bytes = [mods][reserved][key0..key5]
     * With report ID: 9 bytes = [id][mods][reserved][key0..key5] */
    unsigned key_base = 2;
    if (len >= 9) {
        key_base = 3;
    }

    if (len < key_base + 1) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }

    kb_p1 = (KeyboardState){0};
    kb_p2 = (KeyboardState){0};

    unsigned max_keys = (unsigned)len - key_base;
    if (max_keys > 6) {
        max_keys = 6;
    }

    for (unsigned i = 0; i < max_keys; i++) {
        uint8_t key = report[key_base + i];
        if (key == 0) {
            continue;
        }

        if (key == 0x1A) kb_p1.up = true;     // W
        if (key == 0x16) kb_p1.down = true;   // S
        if (key == 0x04) kb_p1.left = true;   // A
        if (key == 0x07) kb_p1.right = true;  // D

        if (key == 0x52) kb_p2.up = true;     // Up
        if (key == 0x51) kb_p2.down = true;   // Down
        if (key == 0x50) kb_p2.left = true;   // Left
        if (key == 0x4F) kb_p2.right = true;  // Right
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    g_usb_hid_ready = true;
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    g_usb_hid_ready = false;
}
