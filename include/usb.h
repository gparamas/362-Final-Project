#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool up, down, left, right;
} KeyboardState;

extern KeyboardState kb_p1;
extern KeyboardState kb_p2;

/* Set by TinyUSB callbacks — visible on VGA for debugging without serial */
extern volatile bool g_usb_device_mounted;
extern volatile bool g_usb_hid_ready;

#endif
