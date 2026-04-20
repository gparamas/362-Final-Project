#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SFX_FLAG_PICKUP,
    SFX_FLAG_CAPTURE,
    SFX_TAGGED,
    SFX_GAME_START,
    SFX_GAME_OVER
} SoundEffect;

typedef enum {
    MUSIC_NONE,
    MUSIC_MENU,
    MUSIC_GAMEPLAY
} MusicTrack;

void audio_init(void);
void audio_play_sfx(SoundEffect sfx);
void audio_play_music(MusicTrack track);
void audio_stop_music(void);

#endif
