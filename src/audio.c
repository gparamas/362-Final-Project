/*
 * Game audio engine — adapted from Ankush's drum_machine.c
 *
 * Uses PWM + DMA ping-pong buffers on Core1 for 44.1 kHz sample playback.
 * Core0 (the game loop) just calls audio_play_sfx() which is non-blocking.
 *
 * Ankush's original code drove the voices from a keypad.  This version
 * replaces that with the SoundEffect enum so the game logic can trigger
 * sounds on events like flag pickup, tagging, and capture.
 */

#include "audio.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "kick_pcm.h"
#include "snare_pcm.h"
#include "hat_pcm.h"
#include "sad_sample_pcm.h"

/* ── config ─────────────────────────────────────────────────────── */
#define AUDIO_GPIO   36u
#define BUF_SAMPLES  2048u
#define SAMPLE_RATE  44100u
#define NUM_VOICES   4

/* ── voice state (Core0 writes, Core1 reads via fill_buf) ─────── */
typedef struct {
    const int16_t    *pcm;
    uint32_t          frames;
    volatile uint32_t pos;
    volatile bool     active;
} voice_t;

static voice_t voices[NUM_VOICES];

/* ── ping-pong buffers + shared flags ─────────────────────────── */
static uint16_t  bufA[BUF_SAMPLES];
static uint16_t  bufB[BUF_SAMPLES];

static volatile bool buf_need[2];
static volatile bool buf_ready[2];
static volatile bool playing;
static volatile bool need_refill;

/* ── PWM / DMA globals ────────────────────────────────────────── */
static uint      pwm_slice;
static uint      pwm_chan;
static uint32_t  pwm_period;
static uint16_t  pwm_top;
static int       dma_ch = -1;
static volatile uint16_t *dma_dst;

/* ── forward declarations ─────────────────────────────────────── */
static void core1_audio_worker(void);
static void audio_dma_isr(void);

/* ── fill_buf — mix active voices into one buffer (Core1 only) ─ */
static void fill_buf(uint16_t *dst, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        int32_t mix   = 0;
        int     count = 0;

        for (int v = 0; v < NUM_VOICES; v++) {
            voice_t *vo = &voices[v];
            if (!vo->active || vo->pos >= vo->frames) continue;
            mix += vo->pcm[vo->pos++];
            count++;
            if (vo->pos >= vo->frames) {
                vo->active = false;
                vo->frames = 0;
            }
        }

        if (count > 1)    mix /= count;
        if (mix >  32767) mix =  32767;
        if (mix < -32768) mix = -32768;

        uint32_t u     = (uint32_t)(mix + 32768);
        uint32_t level = (u * pwm_period) >> 16;
        if (level >= pwm_period) level = pwm_period - 1;
        dst[i] = (uint16_t)level;
    }
}

/* ── init_audio_hw — configure PWM slice + DMA channel ─────── */
static void init_audio_hw(void)
{
    gpio_set_function(AUDIO_GPIO, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(AUDIO_GPIO);
    pwm_chan  = pwm_gpio_to_channel(AUDIO_GPIO);

    uint32_t sys_hz = clock_get_hz(clk_sys);
    uint32_t top    = (sys_hz / SAMPLE_RATE) - 1u;
    pwm_top    = (uint16_t)top;
    pwm_period = top + 1u;

    printf("[audio] sys=%lu Hz  top=%lu  period=%lu\n",
           (unsigned long)sys_hz, (unsigned long)pwm_top,
           (unsigned long)pwm_period);

    pwm_set_clkdiv_int_frac(pwm_slice, 1, 0);
    pwm_set_wrap(pwm_slice, pwm_top);
    pwm_set_chan_level(pwm_slice, pwm_chan, (uint16_t)(pwm_period / 2));
    pwm_set_enabled(pwm_slice, true);

    dma_ch = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg,  true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pwm_get_dreq(pwm_slice));

    volatile uint16_t *cc16 =
        (volatile uint16_t *)&pwm_hw->slice[pwm_slice].cc;
    dma_dst = (pwm_chan == PWM_CHAN_A) ? &cc16[0] : &cc16[1];

    dma_channel_set_irq0_enabled(dma_ch, true);
    irq_set_exclusive_handler(DMA_IRQ_0, audio_dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_configure(dma_ch, &cfg,
                          (void *)dma_dst, bufA, BUF_SAMPLES,
                          false);
}

/* ── DMA ISR — swap buffers on transfer completion ─────────── */
static void audio_dma_isr(void)
{
    dma_hw->ints0 = 1u << dma_ch;

    buf_need[(int)playing] = true;

    bool next = !playing;
    if (buf_ready[(int)next]) {
        buf_ready[(int)next] = false;
        playing = next;
        dma_channel_set_read_addr(dma_ch,
            next ? (void *)bufB : (void *)bufA, true);
    }
    need_refill = true;
}

/* ── Core1 worker — refill ping-pong buffers in background ──── */
static void core1_audio_worker(void)
{
    uint16_t silence = (uint16_t)(pwm_period / 2);
    for (uint32_t i = 0; i < BUF_SAMPLES; i++) {
        bufA[i] = silence;
        bufB[i] = silence;
    }

    buf_ready[0] = true;
    buf_ready[1] = true;
    multicore_fifo_push_blocking(0xCAFE);

    while (true) {
        while (!need_refill) tight_loop_contents();
        need_refill = false;

        for (int b = 0; b < 2; b++) {
            if (buf_need[b]) {
                buf_need[b] = false;
                fill_buf(b == 0 ? bufA : bufB, BUF_SAMPLES);
                buf_ready[b] = true;
            }
        }

        if (!dma_channel_is_busy(dma_ch)) {
            for (int b = 0; b < 2; b++) {
                if (buf_ready[b]) {
                    buf_ready[b] = false;
                    playing      = (bool)b;
                    dma_channel_set_read_addr(dma_ch,
                        b ? (void *)bufB : (void *)bufA, true);
                    break;
                }
            }
        }
    }
}

/* ── trigger_voice — fire a one-shot sample ────────────────── */
static void trigger_voice(int idx)
{
    static const int16_t *const pcm_table[NUM_VOICES] = {
        kick_pcm, snare_pcm, hat_pcm, sad_sample_pcm
    };
    static const uint32_t pcm_lengths[NUM_VOICES] = {
        kick_pcm_len, snare_pcm_len, hat_pcm_len, sad_sample_pcm_len
    };

    if (idx < 0 || idx >= NUM_VOICES) return;

    voice_t *v = &voices[idx];
    v->pcm    = pcm_table[idx];
    v->frames = pcm_lengths[idx];
    v->pos    = 0;
    __dmb();
    v->active = true;
}

/* ================================================================
 * Public API — called from the game loop on Core0
 * ================================================================ */

void audio_init(void)
{
    memset(voices, 0, sizeof(voices));
    init_audio_hw();

    multicore_launch_core1(core1_audio_worker);
    multicore_fifo_pop_blocking();

    buf_ready[0] = false;
    playing      = false;
    dma_channel_set_read_addr(dma_ch, bufA, true);

    printf("[audio] Ready.\n");
}

void audio_play_sfx(SoundEffect sfx)
{
    switch (sfx) {
        case SFX_FLAG_PICKUP:  trigger_voice(0); break;   /* kick   */
        case SFX_TAGGED:       trigger_voice(1); break;   /* snare  */
        case SFX_FLAG_CAPTURE: trigger_voice(2); break;   /* hat    */
        case SFX_GAME_START:   trigger_voice(0); break;   /* kick   */
        case SFX_GAME_OVER:    trigger_voice(3); break;   /* sad    */
    }
}

void audio_play_music(MusicTrack track)
{
    (void)track;
    /* TODO: looping background music — needs a dedicated voice
       that auto-restarts when it reaches the end of its sample. */
}

void audio_stop_music(void)
{
    /* TODO: deactivate the music voice */
}
