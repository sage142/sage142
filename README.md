# Dear ImGui + ONNX Runtime C++ Demo

This repository contains a complete C++ desktop application that combines:

- **Dear ImGui** for the UI
- **ONNX Runtime** for AI inference
- **GLFW + OpenGL** for rendering/windowing

The app is a simple **MNIST digit recognizer**. You draw a digit on a 28x28 canvas, click **Predict**, and the model returns scores for digits `0-9`.

## Project structure

- `src/main.cpp` - UI + ONNX Runtime inference logic
- `CMakeLists.txt` - build configuration (fetches GLFW + ImGui)
- `tools/download_mnist_model.sh` - helper script to download model
- `assets/mnist-8.onnx` - model location (downloaded by script)

---

## Dependencies

### Build/runtime tools

- CMake 3.20+
- C++17 compiler (GCC/Clang/MSVC)
- Git
- OpenGL development headers
- `curl` (for model download script)

### Third-party libraries

- [ONNX Runtime](https://onnxruntime.ai/docs/install/)
- Dear ImGui (fetched by CMake)
- GLFW (fetched by CMake)

---

## Setup

### 1) Clone and enter repo

```bash
git clone <your-repo-url>
cd sage142
```

### 2) Download ONNX Runtime (CPU build)

Download a prebuilt package from ONNX Runtime Releases and extract it somewhere local, e.g.:

```bash
/opt/onnxruntime-linux-x64-1.22.0
```

Set this as `ONNXRUNTIME_DIR`.

### 3) Download MNIST model

```bash
./tools/download_mnist_model.sh
```

### 4) Configure and build

```bash
cmake -S . -B build -DONNXRUNTIME_DIR=/opt/onnxruntime-linux-x64-1.22.0
cmake --build build -j
```

### 5) Run

```bash
./build/digit_demo
```

Or pass a custom model path:

```bash
./build/digit_demo /path/to/model.onnx
```

---

## Notes

- If your platform requires shared library path setup, export it before running:

```bash
export LD_LIBRARY_PATH=/opt/onnxruntime-linux-x64-1.22.0/lib:$LD_LIBRARY_PATH
```

- The app expects an MNIST-like model with input shape `[1,1,28,28]` and an output vector of 10 scores.

---

## Why this is a useful template

You can reuse this structure for many C++ AI desktop apps:

- Replace the MNIST model with your own ONNX model
- Keep Dear ImGui for controls, visualizations, and debug tools
- Add post-processing and domain logic in C++

This gives you a fast, native GUI + AI stack without Python at runtime.
