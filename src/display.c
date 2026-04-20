#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
// 7-segment display message buffer
// Declared as static to limit scope to this file only.
static char msg[8] = {
    0x3F, // seven-segment value of 0
    0x06, // seven-segment value of 1
    0x5B, // seven-segment value of 2
    0x4F, // seven-segment value of 3
    0x66, // seven-segment value of 4
    0x6D, // seven-segment value of 5
    0x7D, // seven-segment value of 6
    0x07, // seven-segment value of 7
};
extern char font[]; // Font mapping for 7-segment display
static int index = 0; // Current index in the message buffer

// We provide you with this function for directly displaying characters.
// This now accounts for the decimal point.
void display_char_print(const char message[]) {
    int dp_found = 0;
    for (int i = 0; i < 8; i++) {
        if (message[i] == '.') {
            // add it to the decimal point bit for prev digit if i > 0
            if (i > 0) {
                msg[i - 1] |= (1 << 7); // Set the decimal point bit
                dp_found = 1; // Mark that we found a decimal point
            }
        }
        else {
            msg[i - dp_found] = font[message[i] & 0xFF];
        }
    }
    if (dp_found) {
        msg[7] = font[message[8] & 0xFF]; // Clear the last character if we found a decimal point
    }
}

/********************************************************* */
// Implement the functions below.


void display_init_pins() {
    int outPin;

    for(outPin = 10; outPin < 21; outPin++) {
        sio_hw->gpio_oe_set = 1u << (outPin);
        pads_bank0_hw->io[outPin] &= ~(PADS_BANK0_GPIO0_OD_BITS);
        pads_bank0_hw->io[outPin] |= (PADS_BANK0_GPIO0_IE_BITS);
        io_bank0_hw->io[outPin].ctrl = GPIO_FUNC_SIO;
        pads_bank0_hw->io[outPin] &= ~(PADS_BANK0_GPIO0_ISO_BITS);
    }
}

void display_isr() {
    hw_clear_bits(&timer1_hw->intr, 1u << 0);
    sio_hw->gpio_clr = 0x7ff << 10;
    sio_hw->gpio_set = (index << 18) | (msg[index] << 10);
    index++;
    if(index>7) {
        index = 0;
    }
    uint64_t target_time = timer1_hw->timerawl;
    timer1_hw->alarm[0] = (uint32_t)(target_time + 3000);
}

void display_init_timer() {
    irq_set_exclusive_handler(TIMER_ALARM_IRQ_NUM(timer1_hw, 0), &display_isr);
    irq_set_enabled(TIMER_ALARM_IRQ_NUM(timer1_hw, 0), true);

    hw_clear_bits(&timer1_hw->intr, TIMER_INTR_ALARM_0_BITS);
    hw_set_bits(&timer1_hw->inte, TIMER_INTR_ALARM_0_BITS);
    uint64_t target_time = timer1_hw->timerawl;
    timer1_hw->alarm[0] = (uint32_t)(target_time + 3000);
}

void display_print(const uint16_t message[]) {
    int i;
    for (i=0; i<9; i++) {
        msg[i] = ((message[i] & (1u << 8)) >> 1) | font[message[i] & 0xFF];
    }
}