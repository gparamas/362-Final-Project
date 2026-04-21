// audio_machine.c
// Single-core audio engine: PWM wrap IRQ fires at 44100 Hz, mixes up to
// 4 voices, writes one sample to the PWM CC register per interrupt.
// No DMA, no core1, no ping-pong buffers.
//
// Keeps out of the way of the VGA driver (which owns DMA channels 0-1 and
// PIO0) and TinyUSB host (which owns USBCTRL_IRQ). We use GPIO 36 for the
// PWM audio output and PWM_IRQ_WRAP_0 for the per-sample interrupt.

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "audio_machine.h"

#include "fah_sample_pcm.h"
#include "snare_pcm.h"
#include "cucaracha_sample_pcm.h"
#include "sad_sample_pcm.h"

// We deliberately don't pull in support.h here. support.h has a tentative
// definition of `wavetable[N]` at file scope, which would create a duplicate
// global if any other translation unit ever includes it.
#define AUDIO_RATE  44100u
#define AUDIO_GPIO  36u
#define NUM_VOICES  4

typedef struct {
    const int16_t    *pcm;
    uint32_t          frames;
    volatile uint32_t pos;
    volatile bool     active;
} voice_t;

static voice_t voices[NUM_VOICES];

static const int16_t * const flash_pcm[NUM_VOICES] = {
    fah_sample_pcm, snare_pcm, cucaracha_sample_pcm, sad_sample_pcm
};
static const uint32_t flash_frames[NUM_VOICES] = {
    fah_sample_pcm_len, snare_pcm_len, cucaracha_sample_pcm_len, sad_sample_pcm_len
};

static uint     pwm_slice;
static uint     pwm_chan;
static uint32_t pwm_period;   // TOP + 1

// Fires at AUDIO_RATE Hz, mixes voices, writes one sample.
static void audio_isr(void) {
    pwm_hw->intr = 1u << pwm_slice;

    int32_t mix   = 0;
    int     count = 0;

    for (int i = 0; i < NUM_VOICES; i++) {
        voice_t *v = &voices[i];
        if (!v->active || v->pos >= v->frames) continue;

        mix += v->pcm[v->pos++];
        count++;

        if (v->pos >= v->frames) v->active = false;
    }

    if (count > 1)    mix /= count;
    if (mix >  32767) mix =  32767;
    if (mix < -32768) mix = -32768;

    uint32_t u     = (uint32_t)(mix + 32768);
    uint32_t level = (u * pwm_period) >> 16;
    if (level >= pwm_period) level = pwm_period - 1;

    pwm_set_chan_level(pwm_slice, pwm_chan, (uint16_t)level);
}

void audio_machine_init(void) {
    gpio_set_function(AUDIO_GPIO, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(AUDIO_GPIO);
    pwm_chan  = pwm_gpio_to_channel(AUDIO_GPIO);

    uint32_t sys_hz = clock_get_hz(clk_sys);
    uint32_t top    = (sys_hz / AUDIO_RATE) - 1u;
    pwm_period      = top + 1u;

    printf("[audio] sys_hz=%lu top=%lu period=%lu\n",
           (unsigned long)sys_hz, (unsigned long)top, (unsigned long)pwm_period);

    pwm_set_clkdiv_int_frac(pwm_slice, 1, 0);
    pwm_set_wrap(pwm_slice, (uint16_t)top);
    pwm_set_chan_level(pwm_slice, pwm_chan, (uint16_t)(pwm_period / 2)); // silence

    pwm_clear_irq(pwm_slice);
    pwm_set_irq0_enabled(pwm_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, audio_isr);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    pwm_set_enabled(pwm_slice, true);

    printf("[audio] ready - %u voices, %u Hz\n", NUM_VOICES, AUDIO_RATE);
}

static void trigger_voice(int idx) {
    voice_t *v = &voices[idx];
    v->pcm    = flash_pcm[idx];
    v->frames = flash_frames[idx];
    v->pos    = 0;
    // Make sure pos=0 and frames are visible before active=true, or the ISR
    // could see a stale position.
    // __asm volatile("dmb" ::: "memory");
    v->active = true;
}

void play_fah_sample(void) { trigger_voice(0); }
void play_snare(void)      { trigger_voice(1); }
void play_cucaracha_sample(void) { trigger_voice(2); }
void play_sad_sample(void) { trigger_voice(3); }
