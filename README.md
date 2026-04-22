# Sage142 Engine

A lightweight, modular C++ game engine scaffold using:

- **OpenGL** for rendering
- **Lua 5.4** for gameplay scripting
- A configurable **One-Click Publish** pipeline

> This is a production-ready starting point focused on maintainable architecture and clear extension points.

## Features

- Modular subsystem interfaces (`IRenderer`, `IScriptSystem`, `IPublisher`)
- GLFW + GLAD based OpenGL context
- Lua 5.4 binding layer with a simple API (`log`, `set_clear_color`)
- Main loop with fixed startup/shutdown ordering
- Publish tool that builds, stages runtime files, and optionally creates a zip archive
- CMake project with sensible defaults and options

## Project Layout

```text
.
├── CMakeLists.txt
├── assets/
│   └── scripts/
│       └── bootstrap.lua
├── include/
│   └── sage/
│       ├── engine.hpp
│       ├── publisher.hpp
│       ├── renderer.hpp
│       └── script_system.hpp
├── src/
│   ├── engine.cpp
│   ├── main.cpp
│   ├── opengl_renderer.cpp
│   ├── publisher.cpp
│   └── lua_script_system.cpp
└── tools/
    └── publish.sh
```

## Build

### Prerequisites

- C++20 compiler
- CMake 3.21+
- OpenGL development package
- GLFW 3.3+
- Lua 5.4 development package
- (Optional) `zip` CLI for archive output

### Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Run

```bash
./build/sage_engine
```

## One-Click Publish

This command will:

1. Configure and build Release output
2. Stage executable + `assets` into `dist/<platform>/`
3. Optionally create `dist/<platform>.zip`

```bash
./tools/publish.sh
```

Environment overrides:

- `BUILD_DIR` (default: `build`)
- `DIST_DIR` (default: `dist`)
- `CREATE_ZIP` (`1` or `0`, default: `1`)

## Lua Scripting

Engine runs `assets/scripts/bootstrap.lua` at startup.

Available API functions exposed to Lua:

- `log(message)`
- `set_clear_color(r, g, b, a)`

Example in bootstrap:

```lua
log("Lua runtime initialized")
set_clear_color(0.08, 0.09, 0.12, 1.0)
```

## Extending the engine

- Add ECS module under `include/sage/ecs` and `src/ecs`
- Add asset pipeline + hot reload module
- Add editor tooling through ImGui module
- Expand publish profiles for Steam/itch.io packaging
