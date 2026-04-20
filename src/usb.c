#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "tusb.h"

typedef struct {
    bool up, down, left, right;
} PlayerState;

PlayerState player1 = {0};  // WASD
PlayerState player2 = {0};  // Arrow keys

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
    if (len < 3) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }

    player1 = (PlayerState){0};
    player2 = (PlayerState){0};

    for (int i = 2; i < 8; i++) {
        uint8_t key = report[i];
        if (key == 0) continue;

        //WASD
        if (key == 0x1A) player1.up    = true;  // W
        if (key == 0x16) player1.down  = true;  // S
        if (key == 0x04) player1.left  = true;  // A
        if (key == 0x07) player1.right = true;  // D

        //  Arrow keys
        if (key == 0x52) player2.up    = true;
        if (key == 0x51) player2.down  = true;
        if (key == 0x50) player2.left  = true;
        if (key == 0x4F) player2.right = true;
    }

    printf("P1: %s%s%s%s  |  P2: %s%s%s%s\n",
        player1.up    ? "W" : "-",
        player1.down  ? "S" : "-",
        player1.left  ? "A" : "-",
        player1.right ? "D" : "-",
        player2.up    ? "^" : "-",
        player2.down  ? "v" : "-",
        player2.left  ? "<" : "-",
        player2.right ? ">" : "-"
    );

    tuh_hid_receive_report(dev_addr, instance);
}

// Required TinyUSB host callbacks
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    tuh_hid_receive_report(dev_addr, instance);  // start receiving
}
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {}

int main(void) {
    stdio_init_all();
    tusb_init();

    while (1) {
        tuh_task();
    }
}
