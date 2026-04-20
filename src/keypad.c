#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <stdio.h>
#include "queue.h"

// Global column variable
int col = -1;

// Global key state
static bool state[16]; // Are keys pressed/released

// Keymap for the keypad
const char keymap[17] = "DCBA#9630852*741";

void keypad_drive_column();
void keypad_isr();

/********************************************************* */
// Implement the functions below.

void keypad_init_pins() {
    int inPin;
    int outPin;
    for(inPin = 2; inPin < 6; inPin++) {
        sio_hw->gpio_oe_clr = 1u << (inPin);
        pads_bank0_hw->io[inPin] &= ~(PADS_BANK0_GPIO0_OD_BITS);
        pads_bank0_hw->io[inPin] |= (PADS_BANK0_GPIO0_IE_BITS);
        io_bank0_hw->io[inPin].ctrl = GPIO_FUNC_SIO;
        pads_bank0_hw->io[inPin] &= ~(PADS_BANK0_GPIO0_ISO_BITS);
    }

    for(outPin = 6; outPin < 10; outPin++) {
        sio_hw->gpio_oe_set = 1u << (outPin);
        pads_bank0_hw->io[outPin] &= ~(PADS_BANK0_GPIO0_OD_BITS);
        pads_bank0_hw->io[outPin] |= (PADS_BANK0_GPIO0_IE_BITS);
        io_bank0_hw->io[outPin].ctrl = GPIO_FUNC_SIO;
        pads_bank0_hw->io[outPin] &= ~(PADS_BANK0_GPIO0_ISO_BITS);
    }
}

void keypad_init_timer() {
    irq_set_exclusive_handler(TIMER_ALARM_IRQ_NUM(timer0_hw, 0), &keypad_drive_column);
    irq_set_enabled(TIMER_ALARM_IRQ_NUM(timer0_hw, 0), true);
    uint64_t target_time = timer0_hw->timerawl;

    irq_set_exclusive_handler(TIMER_ALARM_IRQ_NUM(timer0_hw, 1), &keypad_isr);
    irq_set_enabled(TIMER_ALARM_IRQ_NUM(timer0_hw, 1), true);

    hw_clear_bits(&timer0_hw->intr, TIMER_INTR_ALARM_0_BITS);
    hw_set_bits(&timer0_hw->inte, TIMER_INTR_ALARM_0_BITS);
    timer0_hw->alarm[0] = (uint32_t)(target_time + 1000000);

    hw_clear_bits(&timer0_hw->intr, TIMER_INTR_ALARM_1_BITS);
    hw_set_bits(&timer0_hw->inte, TIMER_INTR_ALARM_1_BITS);
    timer0_hw->alarm[1] = (uint32_t)(target_time + 1010000);
}

void keypad_drive_column() {
    hw_clear_bits(&timer0_hw->intr, 1u << 0);
    col++;
    if(col>3) {
        col=0;
    }
    sio_hw->gpio_clr = 0xf << 6;
    sio_hw->gpio_set = 1u << (col + 6);
    
    uint64_t target_time = timer0_hw->timerawl;
    timer0_hw->alarm[0] = (uint32_t)(target_time + 25000);
}

uint8_t keypad_read_rows() {
    return (sio_hw->gpio_in >> 2) & 0xF;
}

void keypad_isr() {
    hw_clear_bits(&timer0_hw->intr, 1u << 1);
    uint8_t curr_row_pin = keypad_read_rows();
    int row;
    for(row=0; row<4; row++) {
        int curr_bit; 
        curr_bit = curr_row_pin & (1 << row);
        if(curr_bit != 0) {
            if(state[((col % 4)*4 + (row))] == 0) {
                state[((col % 4)*4 + (row))] = 1;
                key_push((1 << 8)|keymap[((col % 4)*4 + (row))]);
            }
        }
        else if(state[((col % 4)*4 + (row))] != 0){
            state[((col % 4)*4 + (row))] = 0;
            key_push((0 << 8)|keymap[((col % 4)*4 + (row))]);
        }
    }
    uint64_t target_time = timer0_hw->timerawl;
    timer_hw->alarm[1] = (uint32_t)(target_time + 25000);
}
