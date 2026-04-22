# Sage142 Engine

A lightweight, self-contained C++20 game engine scaffold using:

- **OpenGL** for rendering
- **GLFW + GLAD** for windowing/context + OpenGL loading
- **Dear ImGui** runtime debug tooling
- A configurable **One-Click Publish** pipeline

> No external scripting languages are required.

## Features

- Modular subsystem interfaces (`IRenderer`, `IPublisher`)
- OpenGL 3.3 core renderer with GLFW + GLAD
- Optional Dear ImGui debug UI
- Main loop with fixed startup/shutdown ordering
- Publish tool that builds, stages runtime files, and optionally creates a zip archive
- CMake project with auto-fetch dependency fallback (`glfw`, `glad`, `imgui`)

## Build

### Prerequisites

- C++20 compiler
- CMake 3.21+
- OpenGL development package
- Git (for CMake FetchContent)
- (Optional) `zip` CLI for archive output

### Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSAGE_FETCH_DEPS=ON
cmake --build build -j
```

## One-Click Publish

```bash
./tools/publish.sh
```

Environment overrides:

- `BUILD_DIR` (default: `build`)
- `DIST_DIR` (default: `dist`)
- `CREATE_ZIP` (`1` or `0`, default: `1`)

## ImGui

The renderer initializes Dear ImGui and shows both a small `Sage Debug Panel` and the ImGui demo window by default.
Use `-DSAGE_ENABLE_IMGUI=OFF` to disable UI overlays.

## Troubleshooting

- **glfw3/glad not found**: keep `SAGE_FETCH_DEPS=ON` (default) to auto-download dependencies.
- **OpenGL not found**: install graphics/OpenGL development libraries for your platform.
