# Relocate Plumber (2D Platformer in C++)

This project is a small C++ 2D platformer inspired by classic side-scrollers.
You control a red-capped plumber, collect coins, avoid enemies, and reach the flag.

A unique mechanic is **relocation portals**: stepping into one portal teleports the
player to a different portal elsewhere in the map.

## Controls

- Move: `A` / `D` or Arrow Keys
- Jump: `W` / `Space` / Up Arrow
- Quit: window close button

## Build

Requirements:
- CMake 3.16+
- C++17 compiler
- SDL2 development package

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/relocate_plumber
```

## Notes

- This is an original demo and does not ship with Nintendo assets.
- The game loop includes gravity, tile collisions, enemies, collectibles,
  lives, score, and a win state.
