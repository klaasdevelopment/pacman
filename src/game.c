#include "game.h"
#include <math.h>
#include <string.h>
const int dirX[5] = {0, -1, 0, 1, 0}, dirY[5] = {-1, 0, 1, 0, 0};
static const char *maze[MH] = {
    "############################", "#............##............#", "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#", "#.####.#####.##.#####.####.#", "#..........................#",
    "#.####.##.########.##.####.#", "#.####.##.########.##.####.#", "#......##....##....##......#",
    "######.##### ## #####.######", "     #.##### ## #####.#     ", "     #.##          ##.#     ",
    "     #.## ###--### ##.#     ", "######.## #      # ##.######", "      .   #      #   .      ",
    "######.## #      # ##.######", "     #.## ######## ##.#     ", "     #.##          ##.#     ",
    "     #.## ######## ##.#     ", "######.## ######## ##.######", "#............##............#",
    "#.####.#####.##.#####.####.#", "#.####.#####.##.#####.####.#", "#o..##.......  .......##..o#",
    "###.##.##.########.##.##.###", "###.##.##.########.##.##.###", "#......##....##....##......#",
    "#.##########.##.##########.#", "#.##########.##.##########.#", "#..........................#",
    "############################"};
static int wrap(int x) { return (x + MW) % MW; }
bool game_passable(const Game *g, int x, int y, bool ghost) {
    if (y < 0 || y >= MH)
        return false;
    if (x < 0 || x >= MW) {
        if (y != 14)
            return false;
        x = wrap(x);
    }
    /* Blank space outside the maze is decorative, not a corridor. */
    if (y >= 9 && y <= 19 && y != 14 && (x < 5 || x > 22))
        return false;
    return g->map[y][x] != '#' && (ghost || g->map[y][x] != '-');
}
static void actors(Game *g) {
    g->player = (Actor){13, 23, LEFT, 0, false, 0};
    for (int i = 0; i < 4; i++)
        g->ghosts[i] = (Actor){i ? 12 + i : 13, i ? 14 : 11, LEFT, 0, false, (float)i * 2};
    g->frightened = 0;
    g->modeTime = 0;
    g->mode = 0;
    g->timer = 2;
    g->phase = READY;
}
static void board(Game *g) {
    for (int y = 0; y < MH; y++)
        memcpy(g->map[y], maze[y], MW + 1);
    /* Additional layouts preserve the arcade silhouette and open new loops. */
    int variant = (g->layout + g->level - 1) % 3;
    if (variant >= 1)
        for (int y = 2; y <= 4; y++) {
            g->map[y][9] = '.';
            g->map[y][18] = '.';
        }
    if (variant == 2)
        for (int y = 24; y <= 28; y++) {
            g->map[y][12] = '.';
            g->map[y][15] = '.';
        }
    g->dots = 0;
    g->eaten = 0;
    g->fruit = 0;
    for (int y = 0; y < MH; y++)
        for (int x = 0; x < MW; x++)
            if (g->map[y][x] == '.' || g->map[y][x] == 'o')
                g->dots++;
    actors(g);
}
void game_init(Game *g, int layout) {
    memset(g, 0, sizeof(*g));
    g->layout = layout;
    g->level = 1;
    g->lives = 3;
    g->rng = 12345;
    board(g);
    g->phase = TITLE;
}
void game_start(Game *g) {
    int best = g->best, layout = g->layout;
    game_init(g, layout);
    g->best = best;
    g->phase = READY;
    g->timer = 4;
    g->event = E_START;
}
static void score(Game *g, int n) {
    g->score += n;
    if (g->score > g->best)
        g->best = g->score;
    if (!g->extra && g->score >= 10000) {
        g->lives++;
        g->extra = true;
    }
}
void game_target(const Game *g, int i, int *x, int *y) {
    static const int cx[4] = {25, 2, 27, 0}, cy[4] = {-3, -3, 32, 32};
    const Actor *p = &g->player, *a = &g->ghosts[i];
    if (a->eyes) {
        *x = 13;
        *y = 14;
        return;
    }
    if (g->mode % 2 == 0) {
        *x = cx[i];
        *y = cy[i];
        return;
    }
    *x = p->x;
    *y = p->y;
    if (i == 1) {
        *x += 4 * dirX[p->dir];
        *y += 4 * dirY[p->dir];
        if (p->dir == UP)
            *x -= 4;
    }
    if (i == 2) {
        int px = p->x + 2 * dirX[p->dir], py = p->y + 2 * dirY[p->dir];
        if (p->dir == UP)
            px -= 2;
        *x = 2 * px - g->ghosts[0].x;
        *y = 2 * py - g->ghosts[0].y;
    }
    if (i == 3 && (a->x - p->x) * (a->x - p->x) + (a->y - p->y) * (a->y - p->y) < 64) {
        *x = cx[i];
        *y = cy[i];
    }
}
static bool can(const Game *g, const Actor *a, Direction d, bool ghost) {
    if (d == NONE)
        return false;
    int nx = a->x + dirX[d], ny = a->y + dirY[d];
    if (ghost && !a->eyes && a->y == 11 && ny == 12 && (nx == 13 || nx == 14))
        return false;
    return game_passable(g, nx, ny, ghost);
}
static Direction choose(Game *g, int i) {
    Actor *a = &g->ghosts[i];
    int tx, ty;
    game_target(g, i, &tx, &ty);
    if (!a->eyes && a->y >= 12 && a->y <= 15 && a->x >= 11 && a->x <= 16) {
        tx = 13;
        ty = 11;
    }
    Direction best = NONE;
    int cost = 1000000;
    for (Direction d = UP; d <= RIGHT; d++) {
        if (d == (a->dir + 2) % 4 || !can(g, a, d, true))
            continue;
        int dx = a->x + dirX[d] - tx, dy = a->y + dirY[d] - ty, value = dx * dx + dy * dy;
        if (g->frightened > 0 && !a->eyes) {
            g->rng = g->rng * 1664525u + 1013904223u;
            value = (int)(g->rng % 1000);
        }
        if (value < cost) {
            cost = value;
            best = d;
        }
    }
    if (best == NONE && can(g, a, (a->dir + 2) % 4, true))
        best = (a->dir + 2) % 4;
    return best;
}
static void reverse(Actor *a) {
    if (a->dir == NONE)
        return;
    if (a->progress > 0) {
        a->x = wrap(a->x + dirX[a->dir]);
        a->y += dirY[a->dir];
        a->progress = 1 - a->progress;
    }
    a->dir = (a->dir + 2) % 4;
}
static void reverse_ghosts(Game *g) {
    for (int i = 0; i < 4; i++)
        if (!g->ghosts[i].eyes && g->ghosts[i].delay <= 0)
            reverse(&g->ghosts[i]);
}
static void consume(Game *g) {
    char *c = &g->map[g->player.y][g->player.x];
    if (*c == '.' || *c == 'o') {
        bool power = *c == 'o';
        *c = ' ';
        g->dots--;
        g->eaten++;
        score(g, power ? 50 : 10);
        g->event |= power ? E_POWER : E_DOT;
        if (power) {
            g->frightened = fmaxf(1, 7 - (g->level - 1) * 0.5f);
            g->combo = 0;
            reverse_ghosts(g);
        }
        if (g->eaten == 70 || g->eaten == 170)
            g->fruit = 10;
        if (!g->dots) {
            g->phase = CLEAR;
            g->timer = 2;
        }
    }
    if (g->fruit > 0 && g->player.y == 17 && (g->player.x == 13 || g->player.x == 14)) {
        static const int values[] = {100,  300,  500,  500,  700,  700, 1000,
                                     1000, 2000, 2000, 3000, 3000, 5000};
        score(g, values[g->level > 13 ? 12 : g->level - 1]);
        g->fruit = 0;
        g->event |= E_FRUIT;
    }
}
static void move(Game *g, Actor *a, float amount, Direction input, int ghost) {
    while (amount > 0) {
        if (a->progress < 0.00001f) {
            if (ghost >= 0)
                a->dir = choose(g, ghost);
            else if (can(g, a, input, false))
                a->dir = input;
            if (!can(g, a, a->dir, ghost >= 0))
                return;
        }
        float step = fminf(amount, 1 - a->progress);
        a->progress += step;
        amount -= step;
        if (a->progress >= 0.99999f) {
            a->x = wrap(a->x + dirX[a->dir]);
            a->y += dirY[a->dir];
            a->progress = 0;
            if (ghost < 0)
                consume(g);
            else if (a->eyes && a->x == 13 && a->y == 14) {
                a->eyes = false;
                a->delay = 1;
                return;
            }
        }
    }
}
void game_update(Game *g, float dt, Direction input) {
    g->event = 0;
    if (g->paused || g->phase == TITLE || g->phase == OVER)
        return;
    if (g->phase != PLAYING) {
        g->timer -= dt;
        if (g->timer <= 0) {
            if (g->phase == READY)
                g->phase = PLAYING;
            else if (g->phase == DYING) {
                if (--g->lives <= 0)
                    g->phase = OVER;
                else
                    actors(g);
            } else if (g->phase == CLEAR) {
                g->level++;
                board(g);
            }
        }
        return;
    }
    g->fruit = fmaxf(0, g->fruit - dt);
    if (g->frightened > 0)
        g->frightened = fmaxf(0, g->frightened - dt);
    else {
        static const float times[] = {7, 20, 7, 20, 5, 20, 5};
        g->modeTime += dt;
        if (g->mode < 7 && g->modeTime >= times[g->mode]) {
            g->mode++;
            g->modeTime = 0;
            reverse_ghosts(g);
        }
    }
    float speed = fminf(9, 6 + (g->level - 1) * 0.25f);
    if (input != NONE && g->player.dir != NONE && input == (g->player.dir + 2) % 4 &&
        g->player.progress > 0)
        reverse(&g->player);
    move(g, &g->player, dt * speed, input, -1);
    if (g->phase != PLAYING)
        return;
    for (int i = 0; i < 4; i++) {
        Actor *a = &g->ghosts[i];
        if (a->delay > 0) {
            a->delay -= dt;
            continue;
        }
        float s = a->eyes ? 12 : g->frightened > 0 ? speed * 0.55f : speed * 0.92f;
        if (a->y == 14 && (a->x < 6 || a->x > 21) && !a->eyes)
            s *= 0.6f;
        move(g, a, dt * s, NONE, i);
        float dx = a->x + dirX[a->dir] * a->progress - g->player.x -
                   dirX[g->player.dir] * g->player.progress;
        float dy = a->y + dirY[a->dir] * a->progress - g->player.y -
                   dirY[g->player.dir] * g->player.progress;
        if (fabsf(dx) > MW / 2)
            dx = MW - fabsf(dx);
        if (dx * dx + dy * dy < 0.55f && !a->eyes) {
            if (g->frightened > 0) {
                a->eyes = true;
                score(g, 200 << g->combo);
                if (g->combo < 3)
                    g->combo++;
                g->event |= E_GHOST;
            } else {
                g->phase = DYING;
                g->timer = 1.5f;
                g->event |= E_DEATH;
                return;
            }
        }
    }
}
