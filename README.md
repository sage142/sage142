# BeatCanvas DAW (C++ + Dear ImGui)

BeatCanvas is a **beginner-friendly custom DAW prototype** built in C++ with a clean producer workflow.

## What it does

- Track-based layered editing (stack clips across multiple tracks)
- Load WAV samples
- Transport: Play / Pause / Stop (timeline playhead preview)
- Clip editing tools:
  - Reverse
  - Chop at a timeline sample position
  - Playback speed changes (0.5x / 1x / 2x)
- Timeline/sequencer view for arranging clips
- Save and load project files (`.beat` text format)
- Export mixed project as MP3 (when `libmp3lame` is available)

## Dependencies

- GLFW
- OpenGL
- Dear ImGui (local source checkout, or auto-fetched)
- Optional: `libmp3lame` for MP3 export

No SDL dependency is required.

## Dear ImGui setup

This project is offline-friendly and expects Dear ImGui sources in:

```text
third_party/imgui/
```

Required files include `imgui.h`, core `.cpp` files, and `backends/imgui_impl_glfw.*`, `backends/imgui_impl_opengl3.*`.
If this folder is missing, CMake can auto-fetch Dear ImGui (enabled by default via `-DBEATCANVAS_FETCH_IMGUI=ON`).

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

If CMake cannot find GLFW (`glfw3Config.cmake`), install your platform's `glfw3` development package
or pass `-DCMAKE_PREFIX_PATH=/path/to/glfw` (or `-Dglfw3_DIR=...`) to `cmake`.
`pkg-config` is optional and only used as a fallback when available.
As a last resort, CMake can auto-fetch GLFW from GitHub (enabled by default via `-DBEATCANVAS_FETCH_GLFW=ON`).
Likewise, Dear ImGui can be auto-fetched with `-DBEATCANVAS_FETCH_IMGUI=ON`.

## Run

```bash
./build/beat_canvas
```

## Beginner notes

- Start by adding a track and loading a WAV sample.
- Place clips by adjusting each clip's `Start sample` value.
- Use clip tools to quickly reshape ideas.
- Save regularly as a `.beat` project.
