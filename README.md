# Sage Paint (C++ / Dear ImGui)

A lightweight Microsoft-Paint-inspired drawing app focused only on **editing directly on canvas** (no save/export/format handling).

## Tech stack

- C++20
- Dear ImGui
- GLFW
- OpenGL
- CMake + FetchContent

> Note: Your request mentioned React + HTML5 Canvas alongside C++ + Dear ImGui. This implementation follows the native C++ + Dear ImGui direction.

## Features included

- Core tools: pencil, round brush, blend brush, eraser, fill bucket (tolerance), color picker.
- Shape tools: line, rectangle, ellipse, polygon.
- Text tool with editable content and size.
- Primary/secondary colors, palette, and quick swap.
- Canvas zoom, grid toggle, snap-to-grid.
- Basic layers with visibility toggle.
- Multi-step undo/redo.
- Helpful assists:
  - Stroke smoothing.
  - Shape correction hint on rough freehand strokes.
  - Symmetry mode (mirror across vertical center).
  - Pressure simulation (speed-based size variation).
  - Perfect line assist (hold Shift while drawing).
  - Blend brush for shading.
  - Replay log capture of strokes.

## Build

```bash
cmake -S . -B build
cmake --build build -j
./build/sage_paint
```

## Notes

- This code is intentionally modular and heavily commented at the function level.
- Selection tools and advanced free-form selection behavior are scaffolded in the UI/tool model, but can be extended further in follow-up iterations.
