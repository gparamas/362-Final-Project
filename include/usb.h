#ifndef USB_H
#define USB_H

#include <stdbool.h>

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
} KeyboardState;

extern volatile KeyboardState kb_p1;
extern volatile KeyboardState kb_p2;

#endif
