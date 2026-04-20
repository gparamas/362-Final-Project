#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "queue.h"
#include "support.h"

const char* username = "patward2";



static int duty_cycle = 0;
static int dir = 0;
static int color = 0;

void display_init_pins();
void display_init_timer();
void display_char_print(const char message[]);
void keypad_init_pins();
void keypad_init_timer();
void init_wavetable(void);
void set_freq(int chan, float f);
extern KeyEvents kev;
void audio_machine(void);
void audio_machine_init(void);
void play_kick(void);
void play_snare(void);
void play_hat(void);
void play_sad_sample(void);

 #define AUDIO_MACHINE

//////////////////////////////////////////////////////////////////////////////
void init_pwm_static(uint32_t period, uint32_t duty_cycle) {
    // fill in
    if(period == 0) {
        period = 1;
    }
    if(period > 0x10000u) {
        period = 0x10000u;
    }
    uint16_t wrap_num = period - 1;
    uint32_t max_level = (uint32_t)wrap_num + 1u;
    if(max_level>0xFFFFu) {
        max_level = 0xFFFFu;
    }
    if(duty_cycle > max_level) {
        duty_cycle = max_level;
    }

    int i;
    for(i=37; i<40; i++) {
        gpio_set_function(i, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(i);
        uint channel_num = pwm_gpio_to_channel(i);
        pwm_set_clkdiv_int_frac4(slice_num, 150, 0);
        pwm_set_wrap(slice_num, wrap_num);
        pwm_set_chan_level(slice_num, channel_num, duty_cycle);
        pwm_set_enabled(slice_num, true);
    }


}

void pwm_breathing() {
    // fill in
    uint32_t mask_ack = pwm_get_irq0_status_mask();
    pwm_hw->intr = mask_ack;
    if(dir==0 && duty_cycle==100) {
        color = (color + 1) % 3;
        dir = 1;
    }
    else if(duty_cycle==0 && dir==1) {
        dir = 0;
    }

    if(dir==0) {
        duty_cycle++;
    }
    else {
        duty_cycle--;
    }

    uint8_t color_gpio[3] = {37, 38, 39};
    uint8_t gpio_num = color_gpio[color%3];
    uint slice_num = pwm_gpio_to_slice_num(gpio_num);
    uint32_t period = (uint32_t)pwm_hw->slice[slice_num].top + 1;
    uint32_t level = (duty_cycle * period) / 100;
    pwm_set_gpio_level(gpio_num, (uint16_t)level);
}

// void init_pwm_irq() {
//     // fill in
//     uint slice_num = pwm_gpio_to_slice_num(37);
//     pwm_clear_irq(slice_num);
//     pwm_set_irq0_enabled(slice_num, true);
//     irq_set_exclusive_handler(PWM_IRQ_WRAP_0, &pwm_breathing);
//     irq_set_enabled(PWM_IRQ_WRAP_0, true);
//     uint32_t current_period = (uint32_t)pwm_hw->slice[slice_num].top + 1;
//     duty_cycle = 100;
//     dir = 1;
    
//     int i;
//     for(i=37; i<40; i++) {
//         uint slice_num = pwm_gpio_to_slice_num(i);
//         uint channel_num = pwm_gpio_to_channel(i);
//         uint32_t slice_period = (uint32_t)pwm_hw->slice[slice_num].top + 1;
//         uint32_t temp_curr_period = current_period;
//         if(temp_curr_period > slice_period) {
//             temp_curr_period = slice_period;
//         }
//         if(temp_curr_period > 0xFFFFu) {
//             temp_curr_period = 0xFFFFu;
//         }
//         pwm_set_chan_level(slice_num, channel_num, temp_curr_period);
//     }

// }

void pwm_audio_handler() {
    // fill in
    uint slice_num = pwm_gpio_to_slice_num(36);
    pwm_hw->intr = (1u << slice_num);
    offset0 += step0;
    offset1 += step1;
    if(offset0 >= (N << 16)) {
        offset0 = offset0 - (N << 16);
    }
    if(offset1 >= (N << 16)) {
        offset1 = offset1 - (N << 16);
    }
    uint samp = wavetable[offset0 >> 16] + wavetable[offset1 >> 16];
    samp = samp/2;
    uint32_t period = 1000000/RATE - 1;
    samp = (samp * period) / (1 << 16);
    uint channel_num = pwm_gpio_to_channel(36);
    pwm_set_chan_level(slice_num, channel_num, (uint16_t)samp);

}

// void init_pwm_audio() {
//     // fill in
//     gpio_set_function(36, GPIO_FUNC_PWM);
//     uint slice_num = pwm_gpio_to_slice_num(36);
//     uint channel_num = pwm_gpio_to_channel(36);
//     pwm_set_clkdiv_int_frac4(slice_num, 150, 0);
//     uint period = 1000000/RATE - 1;
//     pwm_set_wrap(slice_num, period);
//     pwm_set_chan_level(slice_num, channel_num, 0);
//     init_wavetable();
//     pwm_clear_irq(slice_num);
//     pwm_set_irq0_enabled(slice_num, true);
//     irq_set_exclusive_handler(PWM_IRQ_WRAP_0, &pwm_audio_handler);
//     irq_set_enabled(PWM_IRQ_WRAP_0, true);
//     pwm_set_enabled(slice_num, true);

// }



// int main()
// {
//     // Configures our microcontroller to 
//     // communicate over UART through the TX/RX pins
//     stdio_init_all();

//     #ifdef AUDIO_MACHINE
//         // audio_machine();
//         audio_machine_init();
//         // play_kick();
//         // sleep_ms(500);
//         // play_snare();
//         // sleep_ms(500);
//         // play_hat();
//         // sleep_ms(500);
//         play_sad_sample();
//         sleep_ms(500);
//     #endif

//     while (true) {
//         printf("Hello, world!\n");
//         sleep_ms(1000);
//     }

//     for(;;);
//     return 0;
// }
