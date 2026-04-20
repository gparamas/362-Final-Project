#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "tusb.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "vga_graphics.h"
#include "ctf.h"
#include "usb.h"
#include "audio.h"

/*=== SD card SPI pin config ===*/
#define SD_MISO 16
#define SD_CS   17
#define SD_SCK  18
#define SD_MOSI 19

/*=== game tuning ===*/
#define TICK_MS         33      /* ~30 fps */
#define ROUND_TIME_MS   120000  /* 2-minute rounds */
#define TAG_RANGE       8       /* pixels of proximity for a tag */
#define CAPTURE_PAUSE   1500    /* ms pause after a capture */

/*=== game states ===*/
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

/*=== forward declarations ===*/
void init_spi_sdcard(void);
void disable_sdcard(void);
void enable_sdcard(void);
void sdcard_io_high_speed(void);
void init_sdcard_io(void);
void init_uart(void);
void init_uart_irq(void);

static void draw_title(void);
static void draw_hud(int score1, int score2, int remaining_s);
static int  any_key_pressed(void);
static void wait_for_keypress(void);
static void wait_for_release_then_press(void);
static int  players_in_tag_range(void);
static void handle_tagging(int *score1, int *score2);
static void handle_flag_pickup(void);
static int  handle_capture(int *score1, int *score2);
static void reset_after_capture(void);

/*=== SD glue (same as original) ===*/

void init_spi_sdcard(void) {
    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_init(SD_CS);
    gpio_set_dir(SD_CS, true);
    spi_init(spi0, 400000);
    gpio_put(SD_CS, 1);
}

void disable_sdcard(void) {
    gpio_put(SD_CS, 1);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SIO);
    gpio_set_dir(SD_MOSI, 1);
    gpio_put(SD_MOSI, 1);
}

void enable_sdcard(void) {
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_put(SD_CS, 0);
}

void sdcard_io_high_speed(void) {
    spi_set_baudrate(spi0, 12000000);
}

void init_sdcard_io(void) {
    init_spi_sdcard();
    disable_sdcard();
}

/*=== on-screen debug helper ===*/

static void vga_debug(const char *msg) {
    static int debug_y = 420;
    setTextColor2(WHITE, BLACK);
    setTextSize(1);
    setCursor(5, debug_y);
    writeString((char *)msg);
    debug_y += 10;
    if (debug_y > 470) debug_y = 420;
}

/*=== title screen ===*/

static void draw_title(void) {
    fillRect(0, 0, 640, 480, BLACK);

    setTextColor2(CYAN, BLACK);
    setTextSize(4);
    setCursor(100, 100);
    writeString("CAPTURE");

    setCursor(130, 160);
    writeString("THE FLAG");

    setTextColor2(WHITE, BLACK);
    setTextSize(2);
    setCursor(130, 280);
    writeString("P1: WASD   P2: Arrows");

    setTextColor2(YELLOW, BLACK);
    setTextSize(2);
    setCursor(150, 350);
    writeString("Press any key...");
}

/*=== HUD overlay ===*/

static void draw_hud(int score1, int score2, int remaining_s) {
    char buf[16];

    setTextSize(1);

    setTextColor2(CYAN, BLACK);
    setCursor(5, 2);
    sprintf(buf, "P1: %d", score1);
    writeString(buf);

    setTextColor2(GREEN, BLACK);
    setCursor(580, 2);
    sprintf(buf, "P2: %d", score2);
    writeString(buf);

    setTextColor2(YELLOW, BLACK);
    setCursor(290, 2);
    sprintf(buf, "%d:%02d", remaining_s / 60, remaining_s % 60);
    writeString(buf);
}

/*=== input helpers ===*/

static int any_key_pressed(void) {
    return kb_p1.up || kb_p1.down || kb_p1.left || kb_p1.right ||
           kb_p2.up || kb_p2.down || kb_p2.left || kb_p2.right;
}

static void wait_for_keypress(void) {
    int poll_count = 0;
    while (!any_key_pressed()) {
        tuh_task();
        poll_count++;
        if (poll_count % 200 == 0) {
            char dbg[40];
            sprintf(dbg, "polling... %d", poll_count);
            vga_debug(dbg);
        }
        sleep_ms(10);
    }
}

static void wait_for_release_then_press(void) {
    while (any_key_pressed()) { tuh_task(); sleep_ms(10); }
    wait_for_keypress();
}

/*=== tag / capture logic ===*/

static int players_in_tag_range(void) {
    return (player1->x - TAG_RANGE < player2->x + player2->size) &&
           (player1->x + player1->size + TAG_RANGE > player2->x) &&
           (player1->y - TAG_RANGE < player2->y + player2->size) &&
           (player1->y + player1->size + TAG_RANGE > player2->y);
}

static void handle_tagging(int *score1, int *score2) {
    (void)score1; (void)score2;
    if (!players_in_tag_range()) return;

    int p1_center = player1->x + player1->size / 2;
    int p2_center = player2->x + player2->size / 2;

    if (p1_center >= MIDLINE_PX) {
        audio_play_sfx(SFX_TAGGED);
        if (player1->hasFlag) {
            hasFlag(player1, 0);
            resetFlag(flag2, FLAG2_X, FLAG2_Y);
        }
        resetPlayer(player1, P1_SPAWN_X, P1_SPAWN_Y);
    }

    if (p2_center < MIDLINE_PX) {
        audio_play_sfx(SFX_TAGGED);
        if (player2->hasFlag) {
            hasFlag(player2, 0);
            resetFlag(flag1, FLAG1_X, FLAG1_Y);
        }
        resetPlayer(player2, P2_SPAWN_X, P2_SPAWN_Y);
    }
}

static void handle_flag_pickup(void) {
    if (!player1->hasFlag && touchingFlag(player1, flag2)) {
        hasFlag(player1, 1);
        audio_play_sfx(SFX_FLAG_PICKUP);
    }
    if (!player2->hasFlag && touchingFlag(player2, flag1)) {
        hasFlag(player2, 1);
        audio_play_sfx(SFX_FLAG_PICKUP);
    }
}

static int handle_capture(int *score1, int *score2) {
    if (player1->hasFlag && playerInEndZone(player1)) {
        (*score1)++;
        audio_play_sfx(SFX_FLAG_CAPTURE);

        setTextColor2(CYAN, BLACK);
        setTextSize(3);
        setCursor(170, 220);
        writeString("P1 CAPTURED!");
        sleep_ms(CAPTURE_PAUSE);

        reset_after_capture();
        return 1;
    }
    if (player2->hasFlag && playerInEndZone(player2)) {
        (*score2)++;
        audio_play_sfx(SFX_FLAG_CAPTURE);

        setTextColor2(GREEN, BLACK);
        setTextSize(3);
        setCursor(170, 220);
        writeString("P2 CAPTURED!");
        sleep_ms(CAPTURE_PAUSE);

        reset_after_capture();
        return 1;
    }
    return 0;
}

static void reset_after_capture(void) {
    resetPlayer(player1, P1_SPAWN_X, P1_SPAWN_Y);
    resetPlayer(player2, P2_SPAWN_X, P2_SPAWN_Y);
    hasFlag(player1, 0);
    hasFlag(player2, 0);
    resetFlag(flag1, FLAG1_X, FLAG1_Y);
    resetFlag(flag2, FLAG2_X, FLAG2_Y);
}

/*=== main ===*/

int main(void) {
    /*
     * Init order modeled after Ammar's working standalone USB code:
     * stdio_init_all() then tusb_init() immediately.
     * VGA and other peripherals come after.
     */
    stdio_init_all();
    tusb_init();

    initVGA();
    dma_channel_claim(0);
    dma_channel_claim(1);

    init_uart();
    init_uart_irq();
    init_sdcard_io();

    /* Audio disabled for debugging USB — re-enable once keyboard works */
    // audio_init();

    loadDefaultMap();

    /* --- game loop --- */
    GameState state = STATE_TITLE;
    int score1 = 0, score2 = 0;

    for (;;) {
        switch (state) {

        /* ---------- TITLE ---------- */
        case STATE_TITLE: {
            draw_title();
            vga_debug("Waiting for USB keyboard...");
            wait_for_release_then_press();
            state = STATE_PLAYING;
            break;
        }

        /* ---------- PLAYING ---------- */
        case STATE_PLAYING: {
            score1 = 0;
            score2 = 0;

            initCTF();
            dispMap(game_map);

            absolute_time_t round_start = get_absolute_time();
            int game_running = 1;

            while (game_running) {
                absolute_time_t frame_start = get_absolute_time();

                tuh_task();

                /* --- move player 1 (WASD) --- */
                if (kb_p1.up)    moveUp(player1, MOVE_SPEED);
                if (kb_p1.down)  moveDown(player1, MOVE_SPEED);
                if (kb_p1.left)  moveLeft(player1, MOVE_SPEED);
                if (kb_p1.right) moveRight(player1, MOVE_SPEED);

                /* --- move player 2 (arrows) --- */
                if (kb_p2.up)    moveUp(player2, MOVE_SPEED);
                if (kb_p2.down)  moveDown(player2, MOVE_SPEED);
                if (kb_p2.left)  moveLeft(player2, MOVE_SPEED);
                if (kb_p2.right) moveRight(player2, MOVE_SPEED);

                /* --- game rules --- */
                handle_flag_pickup();
                handle_tagging(&score1, &score2);
                handle_capture(&score1, &score2);

                /* --- timer --- */
                int elapsed_ms = absolute_time_diff_us(round_start,
                                     get_absolute_time()) / 1000;
                int remaining_s = (ROUND_TIME_MS - elapsed_ms) / 1000;
                if (remaining_s < 0) remaining_s = 0;

                draw_hud(score1, score2, remaining_s);

                if (elapsed_ms >= ROUND_TIME_MS) {
                    game_running = 0;
                }

                /* --- frame pacing --- */
                int frame_elapsed_us = (int)absolute_time_diff_us(
                                           frame_start, get_absolute_time());
                int wait_us = (TICK_MS * 1000) - frame_elapsed_us;
                if (wait_us > 0) sleep_us(wait_us);
            }

            state = STATE_GAME_OVER;
            break;
        }

        /* ---------- GAME OVER ---------- */
        case STATE_GAME_OVER: {
            fillRect(100, 160, 440, 200, BLACK);
            setTextSize(3);

            if (score1 > score2) {
                setTextColor2(CYAN, BLACK);
                setCursor(140, 180);
                writeString("PLAYER 1 WINS!");
            } else if (score2 > score1) {
                setTextColor2(GREEN, BLACK);
                setCursor(140, 180);
                writeString("PLAYER 2 WINS!");
            } else {
                setTextColor2(YELLOW, BLACK);
                setCursor(200, 180);
                writeString("DRAW!");
            }

            setTextColor2(WHITE, BLACK);
            setTextSize(2);
            setCursor(120, 260);
            char buf[32];
            sprintf(buf, "Score: %d - %d", score1, score2);
            writeString(buf);

            setCursor(130, 320);
            writeString("Press any key to play again");

            wait_for_release_then_press();
            state = STATE_PLAYING;
            break;
        }
        } /* switch */
    } /* for */
}
