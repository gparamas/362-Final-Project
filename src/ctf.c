#include "ctf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vga_graphics.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"

/*=== globals ===*/
char    flagSprite[450];
Player* player1;
Player* player2;
Flag*   flag1;
Flag*   flag2;
char    game_map[MAP_COLS * MAP_ROWS];

/*=== background / map helpers ===*/

char get_bg_color(short x, short y) {
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return BLACK;
    if (x < ENDZONE_PX || x >= (640 - ENDZONE_PX))  return RED;

    int col = x / TILE_PX;
    int row = y / TILE_PX;
    if (col >= 0 && col < MAP_COLS && row >= 0 && row < MAP_ROWS) {
        if (game_map[col * MAP_ROWS + row] == 1) return BLACK;
    }
    return WHITE;
}

int hits_wall(short x, short y, short size) {
    if (x < 0 || y < 0 || x + size > 640 || y + size > 480) return 1;

    int c0 = x / TILE_PX;
    int c1 = (x + size - 1) / TILE_PX;
    int r0 = y / TILE_PX;
    int r1 = (y + size - 1) / TILE_PX;

    for (int c = c0; c <= c1; c++) {
        for (int r = r0; r <= r1; r++) {
            if (c < 0 || c >= MAP_COLS || r < 0 || r >= MAP_ROWS) return 1;
            if (game_map[c * MAP_ROWS + r] == 1) return 1;
        }
    }
    return 0;
}

/*--- erase a rectangle back to the correct background ---*/
static void erase_rect(short x, short y, short w, short h) {
    for (int px = x; px < x + w; px++) {
        for (int py = y; py < y + h; py++) {
            drawPixel(px, py, get_bg_color(px, py));
        }
    }
}

/*=== initialisation ===*/

void initCTF(void) {
    /* free previous round's allocations (safe if NULL on first call) */
    free(player1); player1 = NULL;
    free(player2); player2 = NULL;
    free(flag1);   flag1   = NULL;
    free(flag2);   flag2   = NULL;

    /* draw end-zone / field background */
    for (int i = 0; i < 480; i++) {
        memset(&vga_data_array[320 * i],
               (char)(RED << 3 | RED), 35);           /* left end-zone  (70 px) */
        memset(&vga_data_array[320 * i + 35], 0xFF, 250); /* white field */
        memset(&vga_data_array[320 * i + 285],
               (char)(RED << 3 | RED), 35);            /* right end-zone (70 px) */
    }

    /* flags — readFlag() must be called BEFORE this (while SD mounted) */
    flag1       = initFlag(FLAG1_X, FLAG1_Y, BLUE);
    flag1->state = ALONE;
    flag2       = initFlag(FLAG2_X, FLAG2_Y, GREEN);
    flag2->state = ALONE;

    /* players */
    player1       = initPlayer(P1_SPAWN_X, P1_SPAWN_Y, PLAYER_SIZE, BLUE);
    player1->flag = flag2;
    player2       = initPlayer(P2_SPAWN_X, P2_SPAWN_Y, PLAYER_SIZE, GREEN);
    player2->flag = flag1;
}

Player* initPlayer(short x, short y, short size, char color) {
    Player* p = malloc(sizeof(Player));
    p->x       = x;
    p->y       = y;
    p->size    = size;
    p->hasFlag = 0;
    p->color   = color;
    p->flag    = NULL;
    for (int px = p->x; px < p->x + p->size; px++)
        for (int py = p->y; py < p->y + p->size; py++)
            drawPixel(px, py, p->color);
    return p;
}

/*=== movement (with wall + player collision) ===*/

int moveRight(Player* player, short dist) {
    short newX = (player->x + dist + player->size > 640)
                 ? 640 - player->size : player->x + dist;
    if (newX == player->x) return 0;

    if (hits_wall(newX, player->y, player->size)) return 1;

    Player test = {newX, player->y, player->size, BLUE, 0, NULL};
    if (player == player1) { if (touchingPlayer(&test, player2)) return 1; }
    else                   { if (touchingPlayer(&test, player1)) return 1; }

    if (player->hasFlag)
        moveFlagTo(player == player1 ? flag2 : flag1,
                   newX + 20, player->y + 20);

    /* erase left strip vacated */
    erase_rect(player->x, player->y, newX - player->x, player->size);
    /* draw new right strip */
    for (int px = player->x + player->size; px < newX + player->size; px++)
        for (int py = player->y; py < player->y + player->size; py++)
            drawPixel(px, py, player->color);

    player->x = newX;
    return 0;
}

int moveLeft(Player* player, short dist) {
    short newX = (player->x - dist < 0) ? 0 : player->x - dist;
    if (newX == player->x) return 0;

    if (hits_wall(newX, player->y, player->size)) return 1;

    Player test = {newX, player->y, player->size, BLUE, 0, NULL};
    if (player == player1) { if (touchingPlayer(&test, player2)) return 1; }
    else                   { if (touchingPlayer(&test, player1)) return 1; }

    if (player->hasFlag)
        moveFlagTo(player == player1 ? flag2 : flag1,
                   newX + 20, player->y + 20);

    /* draw new left strip */
    for (int px = newX; px < player->x; px++)
        for (int py = player->y; py < player->y + player->size; py++)
            drawPixel(px, py, player->color);
    /* erase right strip vacated */
    erase_rect(player->x + player->size - (player->x - newX),
               player->y,
               player->x - newX,
               player->size);

    player->x = newX;
    return 0;
}

int moveUp(Player* player, short dist) {
    short newY = (player->y - dist < 0) ? 0 : player->y - dist;
    if (newY == player->y) return 0;

    if (hits_wall(player->x, newY, player->size)) return 1;

    Player test = {player->x, newY, player->size, BLUE, 0, NULL};
    if (player == player1) { if (touchingPlayer(&test, player2)) return 1; }
    else                   { if (touchingPlayer(&test, player1)) return 1; }

    if (player->hasFlag)
        moveFlagTo(player == player1 ? flag2 : flag1,
                   player->x + 20, newY + 20);

    /* draw new top strip */
    for (int px = player->x; px < player->x + player->size; px++)
        for (int py = newY; py < player->y; py++)
            drawPixel(px, py, player->color);
    /* erase bottom strip vacated */
    erase_rect(player->x,
               player->y + player->size - (player->y - newY),
               player->size,
               player->y - newY);

    player->y = newY;
    return 0;
}

int moveDown(Player* player, short dist) {
    short newY = (player->y + dist + player->size > 480)
                 ? 480 - player->size : player->y + dist;
    if (newY == player->y) return 0;

    if (hits_wall(player->x, newY, player->size)) return 1;

    Player test = {player->x, newY, player->size, BLUE, 0, NULL};
    if (player == player1) { if (touchingPlayer(&test, player2)) return 1; }
    else                   { if (touchingPlayer(&test, player1)) return 1; }

    if (player->hasFlag)
        moveFlagTo(player == player1 ? flag2 : flag1,
                   player->x + 20, newY + 20);

    /* draw new bottom strip */
    for (int px = player->x; px < player->x + player->size; px++)
        for (int py = player->y + player->size; py < newY + player->size; py++)
            drawPixel(px, py, player->color);
    /* erase top strip vacated */
    erase_rect(player->x, player->y, player->size, newY - player->y);

    player->y = newY;
    return 0;
}

void moveTo(Player* player, short x, short y) {
    erase_rect(player->x, player->y, player->size, player->size);
    player->x = x;
    player->y = y;
    for (int px = x; px < x + player->size; px++)
        for (int py = y; py < y + player->size; py++)
            drawPixel(px, py, player->color);
}

/*=== reset helpers ===*/

void resetPlayer(Player* p, short spawn_x, short spawn_y) {
    erase_rect(p->x, p->y, p->size, p->size);
    p->x = spawn_x;
    p->y = spawn_y;
    p->hasFlag = 0;
    for (int px = p->x; px < p->x + p->size; px++)
        for (int py = p->y; py < p->y + p->size; py++)
            drawPixel(px, py, p->color);
}

void resetFlag(Flag* f, short orig_x, short orig_y) {
    erase_rect(f->x, f->y, 30, 30);
    f->x     = orig_x;
    f->y     = orig_y;
    f->state = ALONE;
    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 15; j++) {
            drawPixel(orig_x + j,     orig_y + i,
                      (flagSprite[i * 15 + j] & 0xF0) >> 4);
            drawPixel(orig_x + j + 1, orig_y + i,
                      flagSprite[i * 15 + j] & 0x0F);
        }
}

/*=== flag I/O and rendering ===*/

void readFlag(void) {
    FIL   fptr;
    UINT  read;
    FRESULT fo = f_open(&fptr, "flag.vga", FA_READ);
    FRESULT fr = f_read(&fptr, flagSprite, 450, &read);
    FRESULT fc = f_close(&fptr);
    printf("readFlag: open=%d read=%d bytes=%d close=%d\n", fo, fr, read, fc);
}

Flag* initFlag(short x, short y, char color) {
    Flag* f = malloc(sizeof(Flag));
    f->x     = x;
    f->y     = y;
    f->color = color;
    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 15; j++) {
            drawPixel(x + j,     y + i, (flagSprite[i * 15 + j] & 0xF0) >> 4);
            drawPixel(x + j + 1, y + i,  flagSprite[i * 15 + j] & 0x0F);
        }
    return f;
}

void moveFlagTo(Flag* flag, short x, short y) {
    /* erase old position to correct background */
    erase_rect(flag->x, flag->y, 30, 30);

    if (flag->state == TOUCHED) flag->state = WITH;

    /* draw at new position */
    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 15; j++) {
            drawPixel(x + j,     y + i, (flagSprite[i * 15 + j] & 0xF0) >> 4);
            drawPixel(x + j + 1, y + i,  flagSprite[i * 15 + j] & 0x0F);
        }
    flag->x = x;
    flag->y = y;
}

/*=== spatial queries ===*/

int touchingPlayer(Player* p1, Player* p2) {
    return (p1->x <= p2->x + p2->size) &&
           (p1->x + p1->size >= p2->x) &&
           (p1->y <= p2->y + p2->size) &&
           (p1->y + p1->size >= p2->y);
}

int touchingFlag(Player* p1, Flag* f1) {
    return (p1->x <= f1->x + 30) &&
           (p1->x + p1->size >= f1->x) &&
           (p1->y <= f1->y + 30) &&
           (p1->y + p1->size >= f1->y);
}

int playerInEndZone(Player* p) {
    if (p == player1) return (p->x + p->size / 2) < ENDZONE_PX;
    if (p == player2) return (p->x + p->size / 2) >= (640 - ENDZONE_PX);
    return 0;
}

void hasFlag(Player* p1, int hf) {
    p1->hasFlag    = hf;
    p1->flag->state = hf ? TOUCHED : ALONE;
}

/*=== end screen (non-blocking — caller handles the pause) ===*/

void showEnd(Player* p) {
    setCursor(100, 200);
    setTextSize(5);
    setTextColor2(YELLOW, BLACK);
    char* s = (p == player1) ? "Player 1 Wins!" : "Player 2 Wins!";
    writeString(s);
}

/*=== map I/O ===*/

void readMap(char* buf, char* filename) {
    FIL fp;
    int br;
    FRESULT fo = f_open(&fp, filename, FA_READ);
    FRESULT fr = f_read(&fp, buf, MAP_COLS * MAP_ROWS, &br);
    FRESULT fc = f_close(&fp);
    printf("readMap: open=%d read=%d bytes=%d close=%d\n", fo, fr, br, fc);
}

void dispMap(char* buf) {
    for (int row = 0; row < MAP_ROWS; row++) {
        for (int col = 0; col < MAP_COLS; col++) {
            if (buf[col * MAP_ROWS + row] == 1) {
                int py_start = row * TILE_PX;
                int byte_col = col * (TILE_PX / 2);    /* 10 bytes per tile */
                for (int k = py_start; k < py_start + TILE_PX; k++)
                    memset(&vga_data_array[k * 320 + byte_col],
                           0x00, TILE_PX / 2);          /* 10 bytes = 20 px */
            }
        }
    }
}

void clearMap(void) {
    memset(game_map, 0, sizeof(game_map));
}

void loadDefaultMap(void) {
    memset(game_map, 0, sizeof(game_map));

    /* top & bottom border */
    for (int col = 0; col < MAP_COLS; col++) {
        game_map[col * MAP_ROWS + 0]              = 1;
        game_map[col * MAP_ROWS + (MAP_ROWS - 1)] = 1;
    }
    /* center divider (2 tiles wide, middle 8 rows) */
    for (int row = 8; row < 16; row++) {
        game_map[15 * MAP_ROWS + row] = 1;
        game_map[16 * MAP_ROWS + row] = 1;
    }
    /* left-side cover blocks */
    for (int col = 6; col < 10; col++) {
        game_map[col * MAP_ROWS + 6]  = 1;
        game_map[col * MAP_ROWS + 17] = 1;
    }
    /* right-side cover blocks (mirrored) */
    for (int col = 22; col < 26; col++) {
        game_map[col * MAP_ROWS + 6]  = 1;
        game_map[col * MAP_ROWS + 17] = 1;
    }
}
