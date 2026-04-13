#ifndef CTF_H
#define CTF_H

typedef struct {
    short x;
    short y;
    short size;
    char color;
} Player;

typedef struct {
    short x;
    short y;
    char color;
} Flag;

extern char flagSprite[450];
extern Player* player1;
extern Player* player2;

void initBackground();
void setPlayer1(Player* p1);
void setPlayer2(Player* p2);
Player* initPlayer(short x, short y, short size, char color);
void moveRight(Player* player, short dist);
void moveLeft(Player* player, short dist);
void moveUp(Player* player, short dist);
void moveDown(Player* player, short dist);
void moveTo(Player* player, short x, short y);
int touchingPlayer(Player* p1, Player* p2);
void hasFlag(Player* p1, int hasFlag);


void readFlag();
Flag initFlag(short x, short y, char color);
void moveFlagTo(Flag* flag, short x, short y);
int touchingFlag(Player* p1, Flag* f1);
void hideFlag(Flag* flag);
void showFlag(Flag* flag);

#endif 