#include "game.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TILE 22
#define OX 32
#define OY 112
#define WIDTH (MW * TILE + 64)
#define HEIGHT (MH * TILE + 192)
static const Color ink = {5, 7, 18, 255}, gold = {255, 220, 40, 255};
static Sound sounds[7];
static bool audio;
static const char *soundNames[] = {"chomp", "power", "ghost", "death", "intro", "fruit", "siren"};
static Sound synth(int kind) {
    const int rate = 22050;
    float duration = kind == 4 ? 4.0f : kind == 3 ? 1.4f : kind == 6 ? 0.65f : 0.16f;
    int count = (int)(rate * duration);
    short *samples = malloc((size_t)count * sizeof(short));
    if (!samples)
        return (Sound){0};
    static const int notes[] = {71, 83, 78, 75, 83, 78, 75, 72, 84, 79, 76, 84, 79, 76, 71, 83,
                                78, 75, 83, 78, 75, 75, 76, 77, 77, 78, 79, 79, 80, 83, 0,  0};
    double phase = 0;
    for (int i = 0; i < count; i++) {
        float t = (float)i / rate, u = t / duration, f = 440,
              env = fminf(1, t * 150) * fminf(1, (duration - t) * 90);
        if (kind == 0)
            f = 250 + 900 * fabsf(sinf(u * 3.14159f));
        if (kind == 1)
            f = 300 + 150 * sinf(t * 80);
        if (kind == 2)
            f = 300 + u * 1500;
        if (kind == 3)
            f = 900 * (1 - u) + 90 * sinf(t * 55);
        if (kind == 4) {
            int n = notes[(int)(t * 8) % 32];
            f = n ? 440 * powf(2, (n - 69) / 12.0f) : 0;
            env *= 1 - fmodf(t * 8, 1);
        }
        if (kind == 5)
            f = 700 + 500 * u;
        if (kind == 6)
            f = 190 + 120 * sinf(u * 6.283185f);
        phase += f / rate;
        float wave = 2 * fabsf(2 * (float)(phase - floor(phase)) - 1) - 1;
        samples[i] = (short)(wave * env * 6000);
    }
    Wave w = {(unsigned)count, rate, 16, 1, samples};
    Sound s = LoadSoundFromWave(w);
    free(samples);
    return s;
}
static void sound_init(void) {
    InitAudioDevice();
    audio = IsAudioDeviceReady();
    if (!audio)
        return;
    for (int i = 0; i < 7; i++) {
        char path[128];
        snprintf(path, sizeof(path), "assets/audio/%s.wav", soundNames[i]);
        sounds[i] = FileExists(path) ? LoadSound(path) : synth(i);
    }
    SetMasterVolume(0.55f);
}
static void play(int i) {
    if (audio && IsSoundValid(sounds[i]))
        PlaySound(sounds[i]);
}
static void sound_events(int events) {
    for (int i = 0; i < 6; i++)
        if (events & (1 << i))
            play(i);
}
static Vector2 position(const Actor *a) {
    return (Vector2){OX + (a->x + 0.5f + dirX[a->dir] * a->progress) * TILE,
                     OY + (a->y + 0.5f + dirY[a->dir] * a->progress) * TILE};
}
static void pac(Vector2 p, Direction d, float mouth, float radius) {
    float angle = d == LEFT ? 180 : d == UP ? 270 : d == DOWN ? 90 : 0;
    DrawCircleSector(p, radius, angle + mouth, angle + 360 - mouth, 40, gold);
}
static void ghost(const Game *g, int i, float clock) {
    static const Color colors[] = {
        {255, 62, 72, 255}, {255, 174, 223, 255}, {67, 230, 235, 255}, {255, 173, 76, 255}};
    const Actor *a = &g->ghosts[i];
    Vector2 p = position(a);
    Color c = colors[i];
    bool scared = g->frightened > 0 && !a->eyes;
    if (scared)
        c = g->frightened < 2 && ((int)(clock * 8) % 2) ? RAYWHITE : (Color){42, 65, 235, 255};
    if (!a->eyes) {
        DrawCircleSector((Vector2){p.x, p.y - 1}, 10, 180, 360, 20, c);
        DrawRectangle((int)p.x - 10, (int)p.y - 1, 20, 10, c);
        for (int j = 0; j < 3; j++)
            DrawTriangle((Vector2){p.x - 10 + j * 7, p.y + 7},
                         (Vector2){p.x - 7 + j * 7, p.y + 12 - (int)(clock * 9) % 2 * 3},
                         (Vector2){p.x - 3 + j * 7, p.y + 7}, c);
    }
    if (scared) {
        DrawCircle((int)p.x - 4, (int)p.y - 2, 2, RAYWHITE);
        DrawCircle((int)p.x + 4, (int)p.y - 2, 2, RAYWHITE);
        for (int j = 0; j < 4; j++)
            DrawLine((int)p.x - 6 + j * 3, (int)p.y + 5 + j % 2 * 2, (int)p.x - 3 + j * 3,
                     (int)p.y + 5 + (j + 1) % 2 * 2, RAYWHITE);
    } else
        for (int j = -1; j <= 1; j += 2) {
            DrawEllipse((int)p.x + j * 4, (int)p.y - 2, 3.8f, 5, RAYWHITE);
            DrawCircle((int)p.x + j * 4 + dirX[a->dir] * 2, (int)p.y - 2 + dirY[a->dir] * 2, 2.3f,
                       BLUE);
        }
}
static void centered(const char *s, int y, int size, Color c) {
    DrawText(s, (WIDTH - MeasureText(s, size)) / 2, y, size, c);
}
static void render(const Game *g, float clock) {
    ClearBackground(ink);
    DrawText("1UP", OX, 24, 18, gold);
    DrawText(TextFormat("%06d", g->score), OX, 49, 25, RAYWHITE);
    centered("HIGH SCORE", 24, 18, (Color){157, 167, 195, 255});
    centered(TextFormat("%06d", g->best), 49, 25, RAYWHITE);
    DrawText(TextFormat("LEVEL %02d", g->level), WIDTH - 155, 29, 20, gold);
    DrawText(TextFormat("MAZE %d", (g->layout + g->level - 1) % 3 + 1), WIDTH - 155, 57, 16, GRAY);
    DrawLine(OX, 91, WIDTH - OX, 91, (Color){32, 39, 68, 255});
    Color wall = g->phase == CLEAR && ((int)(clock * 8) % 2) ? RAYWHITE : (Color){42, 71, 240, 255};
    for (int y = 0; y < MH; y++)
        for (int x = 0; x < MW; x++) {
            int px = OX + x * TILE, py = OY + y * TILE;
            char c = g->map[y][x];
            if (c == '#') {
                DrawRectangle(px, py, TILE, TILE, (Color){8, 13, 38, 255});
                if (y == 0 || g->map[y - 1][x] != '#')
                    DrawLineEx((Vector2){px, py + 2}, (Vector2){px + TILE, py + 2}, 2, wall);
                if (y == MH - 1 || g->map[y + 1][x] != '#')
                    DrawLineEx((Vector2){px, py + TILE - 2}, (Vector2){px + TILE, py + TILE - 2}, 2,
                               wall);
                if (x == 0 || g->map[y][x - 1] != '#')
                    DrawLineEx((Vector2){px + 2, py}, (Vector2){px + 2, py + TILE}, 2, wall);
                if (x == MW - 1 || g->map[y][x + 1] != '#')
                    DrawLineEx((Vector2){px + TILE - 2, py}, (Vector2){px + TILE - 2, py + TILE}, 2,
                               wall);
            } else if (c == '.')
                DrawCircle(px + TILE / 2, py + TILE / 2, 2.5f, (Color){255, 192, 157, 255});
            else if (c == 'o' && ((int)(clock * 5) % 2 == 0 || g->paused))
                DrawCircle(px + TILE / 2, py + TILE / 2, 6, (Color){255, 192, 157, 255});
            else if (c == '-')
                DrawRectangle(px, py + 9, TILE, 3, PINK);
        }
    if (g->fruit > 0) {
        int x = OX + 14 * TILE, y = OY + 17 * TILE + 11;
        DrawCircle(x - 4, y + 3, 5, RED);
        DrawCircle(x + 5, y + 5, 5, RED);
        DrawLine(x - 4, y, x + 2, y - 10, GREEN);
        DrawLine(x + 5, y, x + 2, y - 10, GREEN);
    }
    for (int i = 0; i < 4; i++)
        ghost(g, i, clock);
    Vector2 p = position(&g->player);
    float mouth = 8 + 30 * fabsf(sinf(clock * 12));
    if (g->phase == DYING)
        mouth = 5 + (1.5f - g->timer) / 1.5f * 174;
    pac(p, g->player.dir, mouth, 10);
    if (p.x < OX + TILE) {
        p.x += MW * TILE;
        pac(p, g->player.dir, mouth, 10);
    } else if (p.x > OX + (MW - 1) * TILE) {
        p.x -= MW * TILE;
        pac(p, g->player.dir, mouth, 10);
    }
    for (int i = 0; i < g->lives - 1 && i < 7; i++)
        pac((Vector2){OX + 14 + i * 29, HEIGHT - 51}, LEFT, 30, 9);
    DrawText("ARROWS  MOVE     P  PAUSE     M  SOUND", OX, HEIGHT - 25, 14,
             (Color){139, 151, 180, 255});
    if (g->phase == READY)
        centered("READY!", OY + 17 * TILE, 22, gold);
    if (g->phase == TITLE || g->phase == OVER || g->paused) {
        DrawRectangle(70, OY + 8 * TILE, WIDTH - 140, 270, (Color){3, 5, 16, 240});
        centered(g->paused          ? "PAUSED"
                 : g->phase == OVER ? "GAME OVER"
                                    : "PAC-MAN",
                 OY + 9 * TILE, 48, gold);
        centered(g->paused ? "P TO RESUME" : "ENTER TO PLAY", OY + 13 * TILE, 22, RAYWHITE);
        if (!g->paused) {
            centered("1  CLASSIC    2  ARCADES    3  CROSSROADS", OY + 15 * TILE, 16,
                     (Color){160, 177, 218, 255});
            centered("FOUR GHOSTS. ONE MORE DOT.", OY + 17 * TILE, 17, (Color){160, 177, 218, 255});
        }
    }
}
int main(int argc, char **argv) {
    int smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WIDTH, HEIGHT, "PAC-MAN | raylib arcade");
    if (!IsWindowReady()) {
        fprintf(stderr, "Could not open a graphics display. Run from a desktop session.\n");
        return 1;
    }
    SetTargetFPS(60);
    int monitor = GetCurrentMonitor();
    float fit = fminf(1, fminf((GetMonitorWidth(monitor) - 80.0f) / WIDTH,
                               (GetMonitorHeight(monitor) - 100.0f) / HEIGHT));
    SetWindowSize((int)(WIDTH * fit), (int)(HEIGHT * fit));
    RenderTexture2D canvas = LoadRenderTexture(WIDTH, HEIGHT);
    sound_init();
    Game g;
    game_init(&g, 0);
    FILE *f = fopen("highscore.dat", "r");
    if (f) {
        if (fscanf(f, "%d", &g.best) != 1 || g.best < 0)
            g.best = 0;
        fclose(f);
    }
    Direction input = LEFT;
    float accumulator = 0, clock = 0;
    bool muted = false;
    int frames = 0;
    while (!WindowShouldClose()) {
        if ((g.phase == TITLE || g.phase == OVER) && !g.paused) {
            for (int i = 0; i < 3; i++)
                if (IsKeyPressed(KEY_ONE + i)) {
                    int best = g.best;
                    game_init(&g, i);
                    g.best = best;
                }
            if (IsKeyPressed(KEY_ENTER)) {
                game_start(&g);
                input = LEFT;
                sound_events(g.event);
            }
        }
        if (IsKeyPressed(KEY_P) && g.phase != TITLE && g.phase != OVER)
            g.paused = !g.paused;
        if (IsKeyPressed(KEY_M)) {
            muted = !muted;
            if (audio)
                SetMasterVolume(muted ? 0 : 0.55f);
        }
        if (IsKeyPressed(KEY_UP))
            input = UP;
        if (IsKeyPressed(KEY_LEFT))
            input = LEFT;
        if (IsKeyPressed(KEY_DOWN))
            input = DOWN;
        if (IsKeyPressed(KEY_RIGHT))
            input = RIGHT;
        float dt = fminf(GetFrameTime(), 0.1f);
        if (!g.paused)
            clock += dt;
        accumulator += dt;
        while (accumulator >= 1.0f / 120) {
            game_update(&g, 1.0f / 120, input);
            sound_events(g.event);
            accumulator -= 1.0f / 120;
        }
        if (audio) {
            bool active = g.phase == PLAYING && !g.paused;
            int loop = g.frightened > 0 ? 1 : 6;
            if (active && !IsSoundPlaying(sounds[loop]))
                play(loop);
            if (!active) {
                for (int i = 0; i < 7; i++)
                    if (g.paused || i == 1 || i == 6)
                        StopSound(sounds[i]);
            } else
                StopSound(sounds[loop == 1 ? 6 : 1]);
        }
        BeginTextureMode(canvas);
        render(&g, clock);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth() / WIDTH, (float)GetScreenHeight() / HEIGHT);
        DrawTexturePro(canvas.texture, (Rectangle){0, 0, WIDTH, -HEIGHT},
                       (Rectangle){(GetScreenWidth() - WIDTH * scale) / 2,
                                   (GetScreenHeight() - HEIGHT * scale) / 2, WIDTH * scale,
                                   HEIGHT * scale},
                       (Vector2){0, 0}, 0, WHITE);
        EndDrawing();
        if (smoke && ++frames == 3) {
            Image shot = LoadImageFromTexture(canvas.texture);
            ImageFlipVertical(&shot);
            ExportImage(shot, "build/smoke.png");
            UnloadImage(shot);
            break;
        }
    }
    f = fopen("highscore.dat", "w");
    if (f) {
        fprintf(f, "%d\n", g.best);
        fclose(f);
    }
    if (audio) {
        for (int i = 0; i < 7; i++)
            UnloadSound(sounds[i]);
        CloseAudioDevice();
    }
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
