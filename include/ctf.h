#ifndef CTF_H
#define CTF_H

/*--- Spawn / layout constants (pixels) ---*/
#define P1_SPAWN_X  50
#define P1_SPAWN_Y  240
#define P2_SPAWN_X  500
#define P2_SPAWN_Y  240
#define FLAG1_X     50
#define FLAG1_Y     50
#define FLAG2_X     580
#define FLAG2_Y     50
#define ENDZONE_PX  70      // left 0-69, right 570-639
#define MIDLINE_PX  320
#define PLAYER_SIZE 50
#define MOVE_SPEED  5

/*--- Map dimensions (tiles) ---*/
#define MAP_COLS    32
#define MAP_ROWS    24
#define TILE_PX     20      // pixels per tile edge

/*--- Types ---*/
typedef enum { TOUCHED, WITH, ALONE } FLAG_STATE;

typedef struct {
    short x, y;
    char  color;
    FLAG_STATE state;
} Flag;

typedef struct {
    short x, y, size;
    char  color;
    int   hasFlag;
    Flag* flag;
} Player;

/*--- Globals (defined in ctf.c) ---*/
extern char    flagSprite[450];
extern Player* player1;
extern Player* player2;
extern Flag*   flag1;
extern Flag*   flag2;
extern char    game_map[MAP_COLS * MAP_ROWS];

/*--- Initialization ---*/
void initCTF(void);
void readFlag(void);

/*--- Player helpers ---*/
Player* initPlayer(short x, short y, short size, char color);
int  moveRight(Player* player, short dist);
int  moveLeft(Player* player, short dist);
int  moveUp(Player* player, short dist);
int  moveDown(Player* player, short dist);
void moveTo(Player* player, short x, short y);
void resetPlayer(Player* p, short spawn_x, short spawn_y);

/*--- Flag helpers ---*/
Flag* initFlag(short x, short y, char color);
void  moveFlagTo(Flag* flag, short x, short y);
void  resetFlag(Flag* f, short orig_x, short orig_y);
int   touchingFlag(Player* p1, Flag* f1);
void  hasFlag(Player* p1, int hasFlag);

/*--- Collision / spatial queries ---*/
int  touchingPlayer(Player* p1, Player* p2);
int  playerInEndZone(Player* p);
int  hits_wall(short x, short y, short size);
char get_bg_color(short x, short y);

/*--- Map ---*/
void readMap(char* buf, char* filename);
void dispMap(char* buf);
void clearMap(void);
void loadDefaultMap(void);

/*--- End screen (displays text, returns immediately) ---*/
void showEnd(Player* p);

#endif
