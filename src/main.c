// CTF game integration
//
// Combines:
//   - Gautham's VGA driver + CTF game logic (vga_graphics, ctf)
//   - Ammar's TinyUSB host keyboard handler (usb)
//
// Audio and SD-card map loading are intentionally left out for this build
// so we can validate the USB + VGA integration end-to-end first.

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "vga_graphics.h"
#include "ctf.h"
#include "usb.h"

#define PLAYER_SPEED  3
#define FRAME_MS      25    // ~40 FPS game update

// Spawn coordinates for resets. Must match the positions used in initCTF().
#define P1_SPAWN_X    50
#define P1_SPAWN_Y    240
#define P2_SPAWN_X    530
#define P2_SPAWN_Y    240
#define FLAG1_SPAWN_X 50
#define FLAG1_SPAWN_Y 50
#define FLAG2_SPAWN_X 574
#define FLAG2_SPAWN_Y 50

static void return_flag_to_spawn(Flag* f, short sx, short sy) {
    f->state = ALONE;
    moveFlagTo(f, sx, sy);
}

static void tag_player(Player* victim) {
    // Drop the flag if the victim was carrying one, then teleport home.
    if (victim->hasFlag) {
        victim->hasFlag = 0;
        if (victim == player1) return_flag_to_spawn(flag2, FLAG2_SPAWN_X, FLAG2_SPAWN_Y);
        else                   return_flag_to_spawn(flag1, FLAG1_SPAWN_X, FLAG1_SPAWN_Y);
    }
    if (victim == player1) moveTo(victim, P1_SPAWN_X, P1_SPAWN_Y);
    else                   moveTo(victim, P2_SPAWN_X, P2_SPAWN_Y);
}

int main(void) {
    stdio_init_all();
    sleep_ms(500);  // give USB-CDC serial a moment to come up
    printf("\n[main] CTF booting\n");

    initVGA();
    printf("[main] VGA ready\n");

    tusb_init();
    printf("[main] TinyUSB host started\n");

    initCTF();
    // Override flag2 to spawn on player 2's side of the field.
    moveFlagTo(flag2, FLAG2_SPAWN_X, FLAG2_SPAWN_Y);
    printf("[main] CTF ready\n");

    absolute_time_t next_frame = make_timeout_time_ms(FRAME_MS);
    bool game_over = false;

    while (1) {
        // Service USB every loop iteration so host stack keeps up.
        tuh_task();

        if (game_over) continue;
        if (!time_reached(next_frame)) continue;
        next_frame = make_timeout_time_ms(FRAME_MS);

        // Snapshot shared input once per frame.
        KeyboardState p1 = kb_p1;
        KeyboardState p2 = kb_p2;

        // Player 1 (WASD)
        if (p1.up)    moveUp   (player1, PLAYER_SPEED);
        if (p1.down)  moveDown (player1, PLAYER_SPEED);
        if (p1.left)  moveLeft (player1, PLAYER_SPEED);
        if (p1.right) moveRight(player1, PLAYER_SPEED);

        // Player 2 (arrows)
        if (p2.up)    moveUp   (player2, PLAYER_SPEED);
        if (p2.down)  moveDown (player2, PLAYER_SPEED);
        if (p2.left)  moveLeft (player2, PLAYER_SPEED);
        if (p2.right) moveRight(player2, PLAYER_SPEED);

        // Flag pickup: each player grabs the opposing colour flag.
        if (!player1->hasFlag && touchingFlag(player1, flag2)) hasFlag(player1, 1);
        if (!player2->hasFlag && touchingFlag(player2, flag1)) hasFlag(player2, 1);

        // Tagging: if the two players collide, whoever is on their own side
        // tags the intruder.
        if (touchingPlayer(player1, player2)) {
            if      (playerInEndZone(player1)) tag_player(player2);
            else if (playerInEndZone(player2)) tag_player(player1);
        }

        // Win: back to your own end zone carrying the enemy flag.
        if (player1->hasFlag && playerInEndZone(player1)) {
            showEnd(player1);
            game_over = true;
        } else if (player2->hasFlag && playerInEndZone(player2)) {
            showEnd(player2);
            game_over = true;
        }
    }
}
