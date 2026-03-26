# BeatCanvas DAW (C++ + Dear ImGui)

BeatCanvas is a **beginner-friendly custom DAW prototype** built in C++ with a clean producer workflow.

## What it does

- Track-based layered editing (stack clips across multiple tracks)
- Load WAV samples
- Transport: Play / Pause / Stop
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
- SDL2
- Dear ImGui (local source checkout)
- Optional: `libmp3lame` for MP3 export

## Dear ImGui setup

This project is offline-friendly and expects Dear ImGui sources in:

```text
third_party/imgui/
```

Required files include `imgui.h`, core `.cpp` files, and `backends/imgui_impl_glfw.*`, `backends/imgui_impl_opengl3.*`.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/beat_canvas
```

## Beginner notes

- Start by adding a track and loading a WAV sample.
- Place clips by adjusting each clip's `Start sample` value.
- Use clip tools to quickly reshape ideas.
- Save regularly as a `.beat` project.
