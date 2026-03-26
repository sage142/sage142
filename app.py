"""ImGui AI Playground.

A simple pyimgui + GLFW app that runs local text generation using
Hugging Face transformers.
"""

from __future__ import annotations

import threading
import time
from dataclasses import dataclass

import glfw
import imgui
from imgui.integrations.glfw import GlfwRenderer
from OpenGL import GL
from transformers import pipeline

MODEL_NAME = "distilgpt2"
WINDOW_TITLE = "ImGui AI Playground"


@dataclass
class GenerationState:
    prompt: str = "Write a short sci-fi scene about a city powered by mushrooms."
    max_new_tokens: int = 80
    temperature: float = 0.8
    top_p: float = 0.95
    status: str = "Idle"
    output: str = ""
    is_generating: bool = False


class AIEngine:
    def __init__(self, model_name: str = MODEL_NAME) -> None:
        self._model_name = model_name
        self._generator = None
        self._load_error: str | None = None

    def ensure_loaded(self) -> None:
        if self._generator is not None or self._load_error is not None:
            return
        try:
            self._generator = pipeline("text-generation", model=self._model_name)
        except Exception as exc:  # pragmatic app-level error capture
            self._load_error = f"Failed to load model '{self._model_name}': {exc}"

    @property
    def load_error(self) -> str | None:
        return self._load_error

    def generate(self, prompt: str, max_new_tokens: int, temperature: float, top_p: float) -> str:
        if self._generator is None:
            raise RuntimeError("Model is not loaded.")

        generated = self._generator(
            prompt,
            max_new_tokens=max_new_tokens,
            do_sample=True,
            temperature=temperature,
            top_p=top_p,
            num_return_sequences=1,
        )
        return generated[0]["generated_text"]


def init_window(width: int = 1200, height: int = 800):
    if not glfw.init():
        raise RuntimeError("Could not initialize GLFW")

    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, GL.GL_TRUE)

    window = glfw.create_window(width, height, WINDOW_TITLE, None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("Could not create GLFW window")

    glfw.make_context_current(window)
    glfw.swap_interval(1)
    return window


def main() -> None:
    window = init_window()

    imgui.create_context()
    impl = GlfwRenderer(window)

    state = GenerationState()
    engine = AIEngine()
    lock = threading.Lock()

    def generation_worker() -> None:
        with lock:
            state.is_generating = True
            state.status = "Generating..."

        try:
            result = engine.generate(
                prompt=state.prompt,
                max_new_tokens=state.max_new_tokens,
                temperature=state.temperature,
                top_p=state.top_p,
            )
            with lock:
                state.output = result
                state.status = f"Done at {time.strftime('%H:%M:%S')}"
        except Exception as exc:  # keep UI alive even on generation failures
            with lock:
                state.status = f"Error: {exc}"
        finally:
            with lock:
                state.is_generating = False

    engine.ensure_loaded()
    if engine.load_error:
        state.status = engine.load_error

    while not glfw.window_should_close(window):
        glfw.poll_events()
        impl.process_inputs()

        imgui.new_frame()

        imgui.set_next_window_position(20, 20)
        imgui.set_next_window_size(1160, 760, condition=imgui.FIRST_USE_EVER)
        imgui.begin("AI Control Panel", True)

        imgui.text("Local model: " + MODEL_NAME)
        imgui.separator()

        changed, state.prompt = imgui.input_text_multiline(
            "Prompt",
            state.prompt,
            8192,
            width=-1,
            height=160,
        )
        _ = changed

        changed, state.max_new_tokens = imgui.slider_int("Max new tokens", state.max_new_tokens, 8, 300)
        _ = changed
        changed, state.temperature = imgui.slider_float("Temperature", state.temperature, 0.1, 1.8)
        _ = changed
        changed, state.top_p = imgui.slider_float("Top-p", state.top_p, 0.1, 1.0)
        _ = changed

        if state.is_generating:
            imgui.begin_disabled()

        if imgui.button("Generate"):
            if state.prompt.strip():
                thread = threading.Thread(target=generation_worker, daemon=True)
                thread.start()
            else:
                state.status = "Please enter a prompt first."

        if state.is_generating:
            imgui.end_disabled()

        imgui.same_line()
        if imgui.button("Clear Output"):
            state.output = ""
            state.status = "Idle"

        imgui.separator()
        imgui.text_wrapped(f"Status: {state.status}")
        imgui.separator()

        imgui.text("Generated text:")
        imgui.begin_child("OutputPane", 0, 340, border=True)
        imgui.text_wrapped(state.output if state.output else "(No output yet)")
        imgui.end_child()

        imgui.end()

        GL.glClearColor(0.12, 0.12, 0.14, 1)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT)

        imgui.render()
        impl.render(imgui.get_draw_data())
        glfw.swap_buffers(window)

    impl.shutdown()
    glfw.terminate()


if __name__ == "__main__":
    main()
