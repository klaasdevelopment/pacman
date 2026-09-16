# Pac-Man — C / raylib

A playable classic arcade interpretation with four ghosts, the 28 × 31 classic
maze (240 dots and four energizers), and two additional maze variations.
The original arcade game repeats one maze; **Arcades** and **Crossroads** are
new variations, not historically original screens. This follows the classic
arcade direction, rather than the substantially different Atari 2600 port.

## Build and play

Requires a C11 compiler, make, pkg-config, and raylib 5.5 or newer development
headers/libraries. Tested with raylib 6.0 on Linux. Install raylib using your
system package manager, or follow https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux.
Verify installation with `pkg-config --modversion raylib`.

```sh
make
make run
make test
make clean
```

Run from the project directory so optional audio and the local high score resolve
correctly. No downloaded assets are required. The window fits the display and
can be resized; the game retains its aspect ratio.

| Key | Action |
| --- | --- |
| Enter | Start / restart after game over |
| Arrow keys | Move; turns are buffered until the next legal junction |
| P | Pause / resume, including countdowns |
| M | Toggle audio |
| 1 / 2 / 3 | Choose starting maze on the title / game-over screen |
| Escape | Exit |

Completing a maze advances the level and cycles the layouts. Three starting
lives, one extra life at 10,000 points, 10-point dots, 50-point power pellets,
200/400/800/1600-point ghost chains, and level-dependent fruit scores are included.
The high score is saved in `highscore.dat` (ignored by Git).

## Ghost behavior and fidelity

Blinky targets Pac-Man; Pinky targets four tiles ahead (including the original
upward targeting offset); Inky uses a doubled vector from Blinky to two tiles
ahead; Clyde chases outside eight tiles and retreats inside that radius.
Ghosts alternate scatter/chase, avoid reversing at ordinary junctions, use the
up/left/down/right tie priority, slow in tunnels, become frightened at energizers,
and return to the pen as eyes when eaten. Later levels increase movement speed
and shorten frightened time. Movement uses a fixed 120 Hz simulation.

This is a recreation, not a ROM emulator. Speed tables, pen release timers,
scatter schedules on later levels, collision timing, and frightened randomness
are approximations. Arcade restrictions on upward turns, Cruise Elroy, kill
screen behavior, and intermission cinematics are not implemented.

Behavior reference: [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier),
a study based on arcade ROM disassembly and controlled observations.
API reference: [raylib cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html).

## Sound

The game synthesizes a startup melody, chomp, energizer, ghost-eaten, death,
fruit and siren cues. These aim for the arcade character but **are not exact
original recordings or a hardware-accurate sound emulation**. There is a startup
jingle and gameplay siren rather than a continuous background song.

For exact recordings, provide your own WAV files in `assets/audio/`:
`intro.wav`, `chomp.wav`, `power.wav`, `ghost.wav`, `death.wav`, `fruit.wav`,
and `siren.wav`. Each existing file replaces its synthesized cue. Siren and
power cues repeat while their game state is active. WAV files are ignored by Git.
The game runs silently when no audio device is available.

## Verification and troubleshooting

`make test` needs no display or raylib and covers reachability of every pellet
in all layouts, classic pellet count, pause, walls, scores, power pellets, tunnel
wrapping, ghost targets, collisions, lives, progression, and 360,000 simulation
steps. `./build/pacman --smoke` opens a short graphics/audio check and saves
`build/smoke.png`.

On Linux drivers that show corrupted rendering, try:

```sh
LIBGL_ALWAYS_SOFTWARE=1 make run
```

A desktop display is required for the game. The logic tests work headlessly.

## Source organization

- `src/game.c`, `src/game.h`: display-independent rules and maze data.
- `src/main.c`: raylib rendering, keyboard input, generated audio and persistence.
- `tests/test_game.c`: automated gameplay checks.

Pac-Man and its characters belong to their respective owners. This project is
an unofficial recreation, unaffiliated with Atari or Bandai Namco.
