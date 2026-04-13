#include "ctf.h"
#include "vga_graphics.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include <stdio.h>
#include <string.h>
#include "hardware/pio.h"
#include "hardware/dma.h"

char flagSprite[450];
Player* player1;
Player* player2;
Flag* flag1;
Flag* flag2;


void initCTF() {
    memset(vga_data_array, 0xFF, 153600 * sizeof(char));
    readFlag();
    player1 = initPlayer(50, 240, 50, BLUE);
    player2 = initPlayer(200, 240, 50, GREEN);
    flag1 = initFlag(50, 50, BLUE);
    flag2 = initFlag(50, 50, GREEN);
}

Player* initPlayer(short x, short y, short size, char color) {
    Player* p1 = malloc(sizeof(Player));
    p1->x = x;
    p1->y = y;
    p1->size = size;
    p1->color = color;
    for(int x = p1->x; x < p1->x+p1->size; x++) {
        for(int y = p1->y; y < p1->y+p1->size; y++) {
            drawPixel(x, y, p1->color);
        }
    }
    return p1;
}

int moveRight(Player* player, short dist) {
    short newX = player->x + dist + player->size > 640 ? 640 : player->x + dist;
    Player test = {newX, player->y, player->size, BLUE};
    if(player == player1) {
        if(touchingPlayer(&test, player2)) {
            return 1;
        }
    }
    else {
        if(touchingPlayer(&test, player1)) {
            return 1;
        }
    }
    for(int x = player->x; x < newX; x++) {
        for(int y = player->y; y < player->y+player->size; y++) {
            drawPixel(x, y, WHITE);
        }
    }
    for(int x = player->x + player->size; x < player->x + player->size + dist; x++) {
        for(int y = player->y; y < player->y+player->size; y++) {
            drawPixel(x, y, player->color);
        }
    }
    player->x = newX;
    return 0;
}


void moveLeft(Player* player, short dist) {
    short newX = player->x - dist < 0 ? 0 : player->x - dist;

    Player test = {newX, player->y, player->size, BLUE};
    if(player == player1) {
        if(touchingPlayer(&test, player2)) {
            return 1;
        }
    }
    else {
        if(touchingPlayer(&test, player1)) {
            return 1;
        }
    }

    for(int x = newX; x < player->x; x++) {
        for(int y = player->y; y < player->y+player->size; y++) {
            drawPixel(x, y, player->color);
        }
    }
    for(int x = player->x + player->size + dist; x > player->x + player->size - dist; x--) {
        for(int y = player->y; y < player->y+player->size + 1; y++) {
            drawPixel(x, y, WHITE);
        }
    }
    player->x = newX;
    return 0;
}

void moveUp(Player* player, short dist) {
    short newY = player->y - dist < 0 ? 0 : player->y - dist;

    Player test = {player->x, newY, player->size, BLUE};
    if(player == player1) {
        if(touchingPlayer(&test, player2)) {
            return 1;
        }
    }
    else {
        if(touchingPlayer(&test, player1)) {
            return 1;
        }
    }

    for(int x = player->x; x < player->x + player->size; x++) {
        for(int y = player->y; y > newY; y--) {
            drawPixel(x, y, player->color);
        }
    }
    for(int x = player->x; x < player->x + player->size; x++) {
        for(int y = player->y + player->size; y > newY + player->size; y--) {
            drawPixel(x, y, WHITE);
        }
    }
    player->y = newY;
    return 0;
}

void moveDown(Player* player, short dist) {
    short newY = player->y + dist > 480 ? 480 : player->y + dist;

    Player test = {player->x, newY, player->size, BLUE};
    if(player == player1) {
        if(touchingPlayer(&test, player2)) {
            return 1; 
        }
    }
    else {
        if(touchingPlayer(&test, player1)) {
            return 1;
        }
    }

    for(int x = player->x; x < player->x + player->size; x++) {
        for(int y = player->y+player->size; y < newY+player->size; y++) {
            drawPixel(x, y, player->color);
        }
    }
    for(int x = player->x; x < player->x + player->size + 1; x++) {
        for(int y = player->y; y < newY; y++) {
            drawPixel(x, y, WHITE);
        }
    }
    player->y = newY;
    return 0;
}

void moveTo(Player* player, short x, short y) {
    for(int i = player->x; i < player->x + player->size; i++) {
        for(int j = player->y; j < player->y + player->size; j++) {
            drawPixel(i, j, WHITE);
        }
    }

    for(int i = x; i < x + player->size; i++) {
        for(int j = y; j < y + player->size; j++) {
            drawPixel(i, j, player->color);
        }
    }
    player->x = x;
    player->y = y;
}

void readFlag() {
    mount();
    FIL fptr;
    UINT read;
    FRESULT fo = f_open(&fptr, "flag.vga", FA_READ);
    FRESULT fr = f_read(&fptr, flagSprite, 450, &read);
    FRESULT fc = f_close(&fptr);
    printf("%d, %d, %d, %d\n", fo, fr, read, fc);
    unmount();
}

Flag* initFlag(short x, short y, char color) {
    Flag* f1 = malloc(sizeof(Flag));
    f1->x = x;
    f1->y = y;
    f1->color = color;
    for(int i = 0; i < 30; i++) {
        for(int j = 0; j < 15; j++) {
            drawPixel(x + j, y + i, (flagSprite[i * 15 + j] & 0b11110000) >> 4);
            drawPixel(x + 1 + j, y + i, (flagSprite[i * 15 + j] & 0b00001111));
        }
    }
    return f1;
}

void moveFlagTo(Flag* flag, short x, short y) {
    for(int i = flag->y; i < flag->y + 30; i++) {
        for(int j = flag->x; j < flag->x + 30; j++) {
            drawPixel(j, i, WHITE);
        }
    }

    for(int i = 0; i < 30; i++) {
        for(int j = 0; j < 15; j++) {
            drawPixel(x + j, y + i, (flagSprite[i * 15 + j] & 0b11110000) >> 4);
            drawPixel(x + 1 + j, y + i, (flagSprite[i * 15 + j] & 0b00001111));
        }
    }
    flag->x = x;
    flag->y = y;
}

int touchingPlayer(Player* p1, Player* p2) {
    return (p1->x <= p2->x + p2->size) &&
    (p1->x + p1->size >= p2->x) &&
    (p1->y <= p2->y + p2->size) &&
    (p1->y + p1->size >= p2->y);
}

int playerInEndZone(Player* p) {
    if (p == player1) {
        if(p->x < 100) {
            return 1;
        }
        return 0;
    }
    if (p == player2) {
        if(p->x > 540) {
            return 1;
        }
        return 0;
    }
    return -1;
}