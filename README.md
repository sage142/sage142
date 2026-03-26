# ImGui AI Playground (Python)

A tiny desktop app built with **pyimgui** + **GLFW** that lets you prompt a local Hugging Face text-generation model and view results in an ImGui interface.

## What it does

- Renders a Dear ImGui UI in a GLFW/OpenGL window.
- Loads a `transformers` text-generation pipeline.
- Lets you enter a prompt and generation settings.
- Runs generation on a background thread so the UI stays responsive.

## Quick start

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python app.py
```

## Notes

- First run downloads model weights from Hugging Face.
- Default model is `distilgpt2` (small-ish but still requires a bit of RAM).
- You can edit `MODEL_NAME` in `app.py` to use another text-generation model.
