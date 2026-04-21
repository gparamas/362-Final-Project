#ifndef AUDIO_MACHINE_H
#define AUDIO_MACHINE_H

// Single-core audio engine. Output is PWM on GPIO 36; the ISR mixes up to
// four one-shot voices at the sample rate. Call audio_machine_init() once,
// then call any of the play_* functions to (re)trigger a sample.

void audio_machine_init(void);

void play_fah_sample(void);
void play_snare(void);
void play_cucaracha_sample(void);
void play_sad_sample(void);

#endif
