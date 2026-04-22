# Sage142 Engine

A lightweight, modular C++ game engine scaffold using:

- **OpenGL** for rendering
- **Dear ImGui** runtime debug tooling
- A configurable **One-Click Publish** pipeline

## Features

- Modular subsystem interfaces (`IRenderer`, `IPublisher`)
- GLFW based OpenGL context with Dear ImGui debug UI
- Main loop with fixed startup/shutdown ordering
- Publish tool that builds, stages runtime files, and optionally creates a zip archive
- CMake project with sensible defaults, optional auto-fetch of dependencies, and build options

## Project Layout

```text
.
├── CMakeLists.txt
├── include/
│   └── sage/
│       ├── engine.hpp
│       ├── publisher.hpp
│       ├── renderer.hpp
│       └── modules.hpp
├── src/
│   ├── engine.cpp
│   ├── main.cpp
│   ├── opengl_renderer.cpp
│   └── publisher.cpp
└── tools/
    └── publish.sh
```

## Build

### Prerequisites

- C++20 compiler
- CMake 3.21+
- OpenGL development package
- GLFW 3.3+
- Git (for CMake FetchContent to download GLFW/ImGui)
- (Optional) `zip` CLI for archive output

### Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

If `glfw3` is not installed on your machine, CMake will automatically fetch GLFW when `SAGE_FETCH_DEPS=ON` (default).

```bash
cmake -S . -B build -DSAGE_FETCH_DEPS=ON
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
Use `-DSAGE_ENABLE_IMGUI=OFF` at configure time if you want a minimal runtime without UI overlays.

## Troubleshooting

- **glfw3 not found**: keep `SAGE_FETCH_DEPS=ON` (default) to auto-download GLFW, or install GLFW and set `glfw3_DIR`.
- **OpenGL not found**: install graphics/OpenGL development libraries for your platform.
