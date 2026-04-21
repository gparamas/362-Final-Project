// CTF game integration
//
// Combines:
//   - Gautham's VGA driver + CTF game logic (vga_graphics, ctf)
//   - Ammar's TinyUSB host keyboard handler (usb)
//   - Ankush's single-core PWM audio engine (audio_machine)
//   - SD-card SPI helpers for flag sprite / map loading

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "vga_graphics.h"
#include "ctf.h"
#include "usb.h"
#include "audio_machine.h"

#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"

#define PLAYER_SPEED  3
#define FRAME_MS      25    // ~40 FPS game update
#define MIDLINE_X     320   // anything left of this is player 1's half

// initCTF() spawns players/flags at these positions (see ctf.c).
// Re-declared here so the tag/reset helper can send them home.
#define P1_SPAWN_X    90
#define P1_SPAWN_Y    240
#define P2_SPAWN_X    500
#define P2_SPAWN_Y    240
#define FLAG1_SPAWN_X 30
#define FLAG1_SPAWN_Y 240
#define FLAG2_SPAWN_X 590
#define FLAG2_SPAWN_Y 240


#define SD_MISO 16
#define SD_CS 17
#define SD_SCK 18
#define SD_MOSI 19


// ctf.c reads this from SD; we pre-fill it so the flag sprite is visible
// even when no SD card is present. Each byte packs two pixels (high nibble,
// low nibble); 0x55 paints MAGENTA everywhere, which shows up on both
// the white field and the red end zones.
extern char flagSprite[450];

void init_spi_sdcard() {
    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_init(SD_CS);
    gpio_set_dir(SD_CS, true);

    spi_init(spi0, 400000);
    gpio_put(SD_CS, 1);
}

void disable_sdcard() {
    // fill in.
    
    //uint8_t end = 0xFF;
    //uint8_t received = sdcard_write(0xFF);
    ///printf("disable received: 0x%02x\n", received);
    gpio_put(SD_CS, 1);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SIO);
    gpio_set_dir(SD_MOSI, 1);
    gpio_put(SD_MOSI, 1);
}

void enable_sdcard() {
    // fill in.
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_put(SD_CS, 0);
}

void sdcard_io_high_speed() {
    // fill in.
    spi_set_baudrate(spi0, 12000000);
}

void init_sdcard_io() {
    // fill in.
    init_spi_sdcard();
    disable_sdcard();
}


static void return_flag_to_spawn(Flag* f, short sx, short sy) {
    f->state = ALONE;
    moveFlagTo(f, sx, sy);
}

static void tag_player(Player* victim) {
    if(playerInEnemyEndZone(victim))
        return;
    if (victim->hasFlag) {
        victim->hasFlag = 0;
        if (victim == player1) return_flag_to_spawn(flag2, FLAG2_SPAWN_X, FLAG2_SPAWN_Y);
        else                   return_flag_to_spawn(flag1, FLAG1_SPAWN_X, FLAG1_SPAWN_Y);
    }
    if (victim == player1) moveTo(victim, P1_SPAWN_X, P1_SPAWN_Y);
    else                   moveTo(victim, P2_SPAWN_X, P2_SPAWN_Y);
    play_snare();  // tagged!
}

int main(void) {
    stdio_init_all();
    sleep_ms(500);  // give USB-CDC serial a moment to come up
    printf("\n[main] CTF booting\n");

    initVGA();
    printf("[main] VGA ready\n");

    tusb_init();
    printf("[main] TinyUSB host started\n");

    // Pre-populate the flag sprite BEFORE initCTF runs. initCTF calls
    // readFlag(), which tries to read flag.vga off the SD card. With no
    // card the f_read fails silently and leaves our pattern in place.
    memset(flagSprite, 0x55, 450);

    initCTF();
    printf("[main] CTF ready\n");

    audio_machine_init();
    printf("[main] audio ready\n");

    play_kick();  // boot chime / game-start

    absolute_time_t next_frame = make_timeout_time_ms(FRAME_MS);

    while (1) {
        tuh_task();

        if (!time_reached(next_frame)) continue;
        next_frame = make_timeout_time_ms(FRAME_MS);

        KeyboardState p1 = kb_p1;
        KeyboardState p2 = kb_p2;

        int touched = 0;

        // Player 1 (WASD)
        if (p1.up)    touched = moveUp   (player1, PLAYER_SPEED);
        if (p1.down) touched =  moveDown (player1, PLAYER_SPEED);
        if (p1.left)  touched = moveLeft (player1, PLAYER_SPEED);
        if (p1.right) touched = moveRight(player1, PLAYER_SPEED);

        // Player 2 (arrows)
        if (p2.up)    touched = moveUp   (player2, PLAYER_SPEED);
        if (p2.down)  touched = moveDown (player2, PLAYER_SPEED);
        if (p2.left)  touched = moveLeft (player2, PLAYER_SPEED);
        if (p2.right) touched = moveRight(player2, PLAYER_SPEED);

        // Flag pickup: grab the opposing color flag on contact.
        // Using play_kick because hat_pcm has ~30ms of silence at the start
        // and is mostly high-frequency content; kick is loud and obvious.
        if (!player1->hasFlag && touchingFlag(player1, flag2)) {
            hasFlag(player1, 1);
            play_kick();
        }
        if (!player2->hasFlag && touchingFlag(player2, flag1)) {
            hasFlag(player2, 1);
            play_kick();
        }

        // Tagging: on contact, whichever player's center has crossed into
        // enemy territory is the intruder and gets sent home. If both are
        // still on their own side (edges touching at the midline), nothing
        // happens - they just block each other.
        if (touched == 1) {
            short p1_mid = player1->x + player1->size / 2;
            short p2_mid = player2->x + player2->size / 2;

            if (p2_mid < MIDLINE_X) {
                tag_player(player2);        // player 2 in player 1's half
            } else if (p1_mid >= MIDLINE_X) {
                tag_player(player1);        // player 1 in player 2's half
            }
        }

        // Win condition: bring the enemy flag back to your own end zone.
        if (player1->hasFlag && playerInEndZone(player1)) {
            play_sad_sample();
            showEnd(player1);   // blocks forever - game over
        } else if (player2->hasFlag && playerInEndZone(player2)) {
            play_sad_sample();
            showEnd(player2);
        }
    }
}
