# ScreenPilot (C++ AI Screen Controller Prototype)

This is a starter C++ app that simulates an AI assistant which can control desktop workflows for:

- Games (safe repetitive sequences)
- Work tasks (window focus + edits/summaries)
- Chat tasks (draft replies with confirmation)

> It is designed as a **safe prototype**: every task is shown and requires user confirmation before execution.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/screenpilot
```

Then type instructions like:

- `reply on slack`
- `work on spreadsheet`
- `game farming`

Type `quit` to exit.

## How to extend to real screen control

1. Add an `OSAdapter` layer for keyboard/mouse APIs per platform.
2. Integrate OCR or accessibility tree parsing for context.
3. Replace `TinyAssistantBrain` with an LLM/planner backend.
4. Keep the safety gate (approval + emergency stop) before each high-impact action.
