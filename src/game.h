#ifndef GAME_H
#define GAME_H
#include <stdbool.h>
#define MW 28
#define MH 31
typedef enum { UP, LEFT, DOWN, RIGHT, NONE } Direction;
typedef enum { TITLE, READY, PLAYING, DYING, CLEAR, OVER } Phase;
typedef struct {
    int x, y;
    Direction dir;
    float progress;
    bool eyes;
    float delay;
} Actor;
typedef struct {
    char map[MH][MW + 1];
    Actor player, ghosts[4];
    int score, best, lives, level, layout, dots, eaten, combo, event;
    bool paused, extra;
    Phase phase;
    float timer, frightened, modeTime, fruit;
    int mode;
    unsigned rng;
} Game;
enum { E_DOT = 1, E_POWER = 2, E_GHOST = 4, E_DEATH = 8, E_START = 16, E_FRUIT = 32 };
void game_init(Game *g, int layout);
void game_start(Game *g);
void game_update(Game *g, float dt, Direction input);
bool game_passable(const Game *g, int x, int y, bool ghost);
void game_target(const Game *g, int ghost, int *x, int *y);
extern const int dirX[5], dirY[5];
#endif
