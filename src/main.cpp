#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <onnxruntime_cxx_api.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {
constexpr int kCanvasSize = 28;

struct DigitCanvas {
  std::array<float, kCanvasSize * kCanvasSize> pixels{};

  void clear() { pixels.fill(0.0f); }

  void paint(int x, int y, float value = 1.0f) {
    for (int oy = -1; oy <= 1; ++oy) {
      for (int ox = -1; ox <= 1; ++ox) {
        const int nx = x + ox;
        const int ny = y + oy;
        if (nx < 0 || ny < 0 || nx >= kCanvasSize || ny >= kCanvasSize) {
          continue;
        }
        const int idx = ny * kCanvasSize + nx;
        pixels[static_cast<size_t>(idx)] = std::max(pixels[static_cast<size_t>(idx)], value - 0.2f * (std::abs(ox) + std::abs(oy)));
      }
    }
  }
};

class OnnxDigitClassifier {
 public:
  explicit OnnxDigitClassifier(const std::string& model_path)
      : env_(ORT_LOGGING_LEVEL_WARNING, "digit-demo"), session_(nullptr) {
    Ort::SessionOptions options;
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    session_ = Ort::Session(env_, model_path.c_str(), options);

    Ort::AllocatorWithDefaultOptions allocator;
    auto input_name = session_.GetInputNameAllocated(0, allocator);
    input_name_ = input_name.get();
    auto output_name = session_.GetOutputNameAllocated(0, allocator);
    output_name_ = output_name.get();
  }

  std::pair<int, std::vector<float>> predict(const DigitCanvas& canvas) {
    std::vector<float> input(canvas.pixels.begin(), canvas.pixels.end());
    std::array<int64_t, 4> shape{1, 1, kCanvasSize, kCanvasSize};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input.data(), input.size(), shape.data(), shape.size());

    const char* input_names[] = {input_name_.c_str()};
    const char* output_names[] = {output_name_.c_str()};

    auto outputs = session_.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

    float* scores = outputs.front().GetTensorMutableData<float>();
    const auto info = outputs.front().GetTensorTypeAndShapeInfo();
    const auto count = static_cast<size_t>(info.GetElementCount());
    std::vector<float> logits(scores, scores + count);

    const auto best = std::max_element(logits.begin(), logits.end());
    const int prediction = static_cast<int>(std::distance(logits.begin(), best));
    return {prediction, logits};
  }

 private:
  Ort::Env env_;
  Ort::Session session_;
  std::string input_name_;
  std::string output_name_;
};

void DrawPixelCanvas(DigitCanvas& canvas, float scale) {
  ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
  ImVec2 canvas_size(kCanvasSize * scale, kCanvasSize * scale);

  ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(20, 20, 20, 255));

  for (int y = 0; y < kCanvasSize; ++y) {
    for (int x = 0; x < kCanvasSize; ++x) {
      float value = canvas.pixels[static_cast<size_t>(y * kCanvasSize + x)];
      int shade = static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
      ImU32 color = IM_COL32(shade, shade, shade, 255);
      ImVec2 min(canvas_pos.x + x * scale, canvas_pos.y + y * scale);
      ImVec2 max(min.x + scale, min.y + scale);
      draw_list->AddRectFilled(min, max, color);
    }
  }

  draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 255));

  if (hovered && active && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    int x = static_cast<int>((mouse.x - canvas_pos.x) / scale);
    int y = static_cast<int>((mouse.y - canvas_pos.y) / scale);
    canvas.paint(x, y);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string model_path = "assets/mnist-8.onnx";
  if (argc > 1) {
    model_path = argv[1];
  }

  if (!std::filesystem::exists(model_path)) {
    std::cerr << "Model not found at: " << model_path << "\n";
    std::cerr << "Run tools/download_mnist_model.sh or pass model path as argv[1].\n";
    return 1;
  }

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return 1;
  }

#if __APPLE__
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  GLFWwindow* window = glfwCreateWindow(1000, 700, "Dear ImGui + ONNX Runtime (MNIST)", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  DigitCanvas canvas;
  OnnxDigitClassifier classifier(model_path);

  int prediction = -1;
  std::vector<float> logits;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("MNIST Digit Recognizer");
    ImGui::TextWrapped("Draw a digit (0-9) on the 28x28 canvas and click Predict.");
    DrawPixelCanvas(canvas, 14.0f);

    if (ImGui::Button("Predict")) {
      auto result = classifier.predict(canvas);
      prediction = result.first;
      logits = std::move(result.second);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
      canvas.clear();
      prediction = -1;
      logits.clear();
    }

    if (prediction >= 0) {
      ImGui::Separator();
      ImGui::Text("Predicted digit: %d", prediction);
      ImGui::Text("Raw model scores:");
      for (size_t i = 0; i < logits.size(); ++i) {
        ImGui::BulletText("%zu: %.4f", i, logits[i]);
      }
    }

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
