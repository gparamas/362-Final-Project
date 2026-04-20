#ifndef USB_H
#define USB_H

#include <stdbool.h>

typedef struct {
    bool up, down, left, right;
} KeyboardState;

extern KeyboardState kb_p1;
extern KeyboardState kb_p2;

void usb_init(void);
void usb_task(void);

#endif
