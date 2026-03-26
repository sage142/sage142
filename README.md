# Math Generation Garden (C++ / OpenGL)

A tiny arcade game where you move through a mathematically generated garden of orbs.

## Theme

This game is about **generation + math**:

- New collectibles are generated over time.
- Their coordinates follow a **sunflower spiral** (polar equation with the golden angle).
- Colors are generated with trigonometric waves.

## Libraries used

- **GLFW** (window/input)
- **GLM** (math utilities)

> Note: this version intentionally avoids GLEW and uses an OpenGL 2.1 compatibility path.

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

## About the `glfw3Config.cmake` error on Windows

If CMake cannot find `glfw3Config.cmake`, it means GLFW is not installed as a CMake package on your machine.

This project includes a fallback: if `glfw3` or `glm` are not found locally, CMake automatically downloads and builds them with `FetchContent`.

If you still prefer local packages (for example through vcpkg), configure with:

```bash
cmake -S . -B build -DMATH_GAME_USE_SYSTEM_DEPS=ON
```

To always force bundled dependencies, use:

```bash
cmake -S . -B build -DMATH_GAME_USE_SYSTEM_DEPS=OFF
```
