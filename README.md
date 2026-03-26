# Math Generation Garden (C++ / OpenGL)

A tiny arcade game where you move through a mathematically generated garden of orbs.

## Theme

This game is about **generation + math**:

- New collectibles are generated over time.
- Their coordinates follow a **sunflower spiral** (polar equation with the golden angle).
- Colors are generated with trigonometric waves.

## Libraries used

- **GLFW** (window/input)
- **GLEW** (OpenGL function loading)
- **GLM** (math utilities)

## Controls

- Move: `WASD` or arrow keys
- Quit: `Esc`

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/math_generation_game
```

Collect as many generated orbs as possible before the 90 second timer ends.
