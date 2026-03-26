# Math Generation Garden (C++ / OpenGL, 3D)

A small **3D** arcade game where you navigate a ground plane and collect mathematically generated cubes.

## Theme

This game is about **generation + math**:

- New collectibles are generated continuously over time.
- Their 3D coordinates follow a **sunflower spiral** in XZ space using the golden angle.
- Heights and colors are generated with trigonometric functions.

## Libraries used

- **GLFW** (window/input)
- **GLM** (math utilities)
- **GLAD** (OpenGL function loading)
- **Dear ImGui** (live generation + camera controls)

## Controls

- Move on XZ: `WASD` or arrow keys
- Go up: `Space`
- Go down: `Left Shift`
- Rotate camera yaw: `Q` / `E`
- Rotate camera pitch: `R` / `F`
- Quit: `Esc`
- Use the **Generation Controls** ImGui panel to tune spawn rate, spiral radius, pickup radius, height behavior, color frequency, and camera settings in real time.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/math_generation_game
```

Collect as many generated cubes as possible before the 90-second timer ends.

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
