#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void quiet(Game *g) {
    game_init(g, 0);
    game_start(g);
    g->phase = PLAYING;
    for (int i = 0; i < 4; i++)
        g->ghosts[i].delay = 10000;
}
static void connectivity(int layout) {
    Game g;
    game_init(&g, layout);
    bool seen[MH][MW] = {0};
    int qx[MW * MH], qy[MW * MH], head = 0, tail = 1, count = 0, power = 0;
    qx[0] = 13;
    qy[0] = 23;
    seen[23][13] = true;
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        for (int d = 0; d < 4; d++) {
            int nx = (x + dirX[d] + MW) % MW, ny = y + dirY[d];
            if (game_passable(&g, nx, ny, false) && !seen[ny][nx]) {
                seen[ny][nx] = true;
                qx[tail] = nx;
                qy[tail++] = ny;
            }
        }
    }
    for (int y = 0; y < MH; y++) {
        assert(strlen(g.map[y]) == MW);
        for (int x = 0; x < MW; x++)
            if (g.map[y][x] == '.' || g.map[y][x] == 'o') {
                assert(seen[y][x]);
                count++;
                power += g.map[y][x] == 'o';
            }
    }
    assert(count == g.dots);
    assert(power == 4);
    if (!layout)
        assert(count == 244);
}
int main(void) {
    for (int i = 0; i < 3; i++)
        connectivity(i);
    Game g;
    quiet(&g);
    g.paused = true;
    g.event = 0;
    Game copy = g;
    game_update(&g, 1, RIGHT);
    assert(memcmp(&g, &copy, sizeof(g)) == 0);
    g.paused = false;
    g.player = (Actor){1, 1, LEFT, 0, false, 0};
    game_update(&g, 0.1f, LEFT);
    assert(g.player.x == 1 && g.player.progress == 0);
    game_update(&g, 1.0f / 6, RIGHT);
    assert(g.player.x == 2 && g.score == 10 && g.dots == 243);
    g.player = (Actor){1, 2, DOWN, 0, false, 0};
    game_update(&g, 1.0f / 6, DOWN);
    assert(g.frightened > 0 && g.score == 60);
    g.player = (Actor){0, 14, LEFT, 0, false, 0};
    game_update(&g, 1.0f / 6, LEFT);
    assert(g.player.x == 27);
    g.mode = 1;
    g.player = (Actor){10, 10, UP, 0, false, 0};
    int x, y;
    game_target(&g, 0, &x, &y);
    assert(x == 10 && y == 10);
    game_target(&g, 1, &x, &y);
    assert(x == 6 && y == 6);
    g.ghosts[0].x = 8;
    g.ghosts[0].y = 8;
    game_target(&g, 2, &x, &y);
    assert(x == 8 && y == 8);
    g.ghosts[3].x = 10;
    g.ghosts[3].y = 11;
    game_target(&g, 3, &x, &y);
    assert(x == 0 && y == 32);
    quiet(&g);
    g.ghosts[0] = g.player;
    g.ghosts[0].delay = 0;
    game_update(&g, 0.001f, NONE);
    assert(g.phase == DYING);
    game_update(&g, 2, NONE);
    assert(g.lives == 2 && g.phase == READY);
    quiet(&g);
    g.frightened = 5;
    g.ghosts[0] = g.player;
    game_update(&g, 0.001f, NONE);
    assert(g.ghosts[0].eyes && g.score == 200);
    quiet(&g);
    g.dots = 1;
    g.player = (Actor){1, 1, RIGHT, 0, false, 0};
    game_update(&g, 1.0f / 6, RIGHT);
    assert(g.phase == CLEAR);
    game_update(&g, 3, NONE);
    assert(g.level == 2 && g.phase == READY);
    quiet(&g);
    g.score = 9990;
    g.player = (Actor){1, 1, RIGHT, 0, false, 0};
    game_update(&g, 1.0f / 6, RIGHT);
    assert(g.lives == 4 && g.extra);
    /* Long deterministic simulation catches invalid corridor transitions. */
    for (int layout = 0; layout < 3; layout++) {
        game_init(&g, layout);
        game_start(&g);
        for (int n = 0; n < 120000; n++) {
            if (g.phase == OVER)
                game_start(&g);
            game_update(&g, 1.0f / 120, (Direction)((n / 47) % 4));
            assert(game_passable(&g, g.player.x, g.player.y, false));
            for (int i = 0; i < 4; i++)
                assert(game_passable(&g, g.ghosts[i].x, g.ghosts[i].y, true));
        }
    }
    puts("PASS: three reachable mazes, 244 classic pellets, pause, walls, scoring, power, tunnel, "
         "targets, collisions, lives, level progression, simulation");
    return 0;
}
