# ImGui + AI Demo (C++)

A small C++ desktop app that uses:

- **Dear ImGui** for the UI.
- **llama.cpp** as the AI library for local LLM text generation.

## Features

- Input a `.gguf` model path at runtime.
- Type any prompt in the ImGui interface.
- Generate text directly from llama.cpp.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/imgui_llama_chat
```

Then load a local GGUF model file from the UI.

## Notes

- The first CMake configure/build can take a while because dependencies are fetched.
- You need OpenGL support and a valid local model file.
