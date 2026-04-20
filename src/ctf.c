#include "ctf.h"
#include <stdlib.h>
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
int count1 = 0;
int count2 = 0;


void initCTF() {
    for(int i = 0; i < 480; i++) {
        memset(&vga_data_array[320*i], (char)(RED << 3 | RED), 35 * sizeof(char));
        memset(&vga_data_array[320*i+35], 0xFF, 250 * sizeof(char));
        memset(&vga_data_array[320*i+285], (char)(RED << 3 | RED), 35 * sizeof(char));
    }
    readFlag();
    flag1 = initFlag(50, 50, BLUE);
    flag1->state = ALONE;
    flag2 = initFlag(200, 240, GREEN);
    flag2->state = ALONE;
    player1 = initPlayer(50, 240, 50, BLUE);
    player1->flag = flag2;
    player2 = initPlayer(500, 240, 50, GREEN);
    player2->flag = flag1;
    
}

Player* initPlayer(short x, short y, short size, char color) {
    Player* p1 = malloc(sizeof(Player));
    p1->x = x;
    p1->y = y;
    p1->size = size;
    p1->hasFlag = 0;
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

    if(player->hasFlag) {
        moveFlagTo(player == player1 ? flag2 : flag1, newX + 20, player->y + 20);
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


int moveLeft(Player* player, short dist) {
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

    if(player->hasFlag) {
        moveFlagTo(player == player1 ? flag2 : flag1, newX + 20, player->y + 20);
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

int moveUp(Player* player, short dist) {
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

    if(player->hasFlag) {
        moveFlagTo(player == player1 ? flag2 : flag1, player->x + 20, newY + 20);
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

int moveDown(Player* player, short dist) {
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

    if(player->hasFlag) {
        moveFlagTo(player == player1 ? flag2 : flag1, player->x + 20, newY + 20);
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
    // SD-free default: clear the sprite buffer. initFlag/moveFlagTo draw
    // the flag as a solid colored rectangle instead of decoding this buffer,
    // so we never need to read flag.vga off an SD card.
    for (int i = 0; i < 450; i++) {
        flagSprite[i] = 0;
    }
}

Flag* initFlag(short x, short y, char color) {
    Flag* f1 = malloc(sizeof(Flag));
    f1->x = x;
    f1->y = y;
    f1->color = color;
    f1->state = ALONE;
    fillRect(x, y, 16, 30, color);
    return f1;
}

void moveFlagTo(Flag* flag, short x, short y) {
    // Erase the flag at its old spot with whatever color should be there:
    //   - the carrying player's color if the flag is stuck to the player
    //   - white (the field) otherwise
    char bg = WHITE;
    if (flag == flag2 && flag->state == WITH) bg = player1->color;
    else if (flag == flag1 && flag->state == WITH) bg = player2->color;
    fillRect(flag->x, flag->y, 16, 30, bg);

    if (flag->state == TOUCHED) flag->state = WITH;

    fillRect(x, y, 16, 30, flag->color);
    flag->x = x;
    flag->y = y;
}

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

void hasFlag(Player* p1, int hasFlag) {
    p1->hasFlag = hasFlag;
    p1->flag->state = hasFlag ? TOUCHED : ALONE;
}

void showEnd(Player* p) {
    setCursor(100, 200);
    setTextSize(5);
    setTextColor2(BLACK, WHITE);
    char* s = (p == player1) ? "Player 1 Wins!" : "Player 2 Wins!";
    writeString(s);
    // Non-blocking: the caller decides what to do next (freeze, restart, etc.).
}

void readMap(char* buf, char* filename) {
    FIL fp;
    int br;
    FRESULT fo = f_open(&fp, filename, FA_READ);
    FRESULT fr = f_read(&fp, buf, 768, &br);
    FRESULT fc = f_close(&fp);
    printf("%d, %d, %d, %d\n", fo, fr, br, fc);
}

void dispMap(char* buf) {
    for(int i = 0; i < 24; i++) {
        for(int j = 0; j < 32; j++) {
            if(buf[j*24+i] == 1) {
                for(int k = i*20; k < i*20+20; k++)
                    memset(&vga_data_array[k * 320 + j * 10], 0x00, sizeof(char) * 20);
            }
        }
    }
}