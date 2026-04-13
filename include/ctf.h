#ifndef CTF_H
#define CTF_H

typedef struct {
    short x;
    short y;
    short size;
    char color;
} Player;
//There are two global player instances, player1 and player2. You need to use these. call initCTF() at the beginning to initialize them

typedef struct {
    short x;
    short y;
    char color;
} Flag;
//There are two global flag instances, flag1 and flag2. You need to use these. Call initCTF() at the beginning to initialize them

extern char flagSprite[450];
extern Player* player1;
extern Player* player2;
extern Flag* flag1;
extern Flag* flag2;


void initCTF(); // call at the start to initialize background, player1, flag1, player2, flag2

Player* initPlayer(short x, short y, short size, char color); // dont worry about this

int moveRight(Player* player, short dist); //moves player right by dist. Will not move if the movement will cause collision. if collision, returns 1, otherwise 0. same for other movement functions
int moveLeft(Player* player, short dist); //moves player left by dist. Will not move if the movement will cause collision.
int moveUp(Player* player, short dist); //moves player up by dist. Will not move if the movement will cause collision.
int moveDown(Player* player, short dist); //moves player down by dist. Will not move if the movement will cause collision.
void moveTo(Player* player, short x, short y); //moves player to location x, y (x ranges from 0 to 640 (left to right), y ranges from 0 to 480 (top to bottom)) 
                                                //WILL MOVE EVEN IF COLLISION WILL HAPPEN, WHATEVER IS BEING COLLIDED WITH WILL DISAPPEAR
int touchingPlayer(Player* p1, Player* p2); //developer function, dont use this. use values you get from move commands
void hasFlag(Player* p1, int hasFlag); //if player picks up his flag, call this with hasflag = 1. If he loses flag, call with hasflag = 0.


void readFlag(); // dev function
Flag* initFlag(short x, short y, char color); // dev function
void moveFlagTo(Flag* flag, short x, short y); //moves flag to (x, y)
int touchingFlag(Player* p1, Flag* f1); //returns 1 if player is touching flag, 0 otherwise
void hideFlag(Flag* flag); //hide flag on the map (if player picks it up, call hasFlag and this)
void showFlag(Flag* flag); //show flag (if player drops flag, call hasFlag and this)

#endif 