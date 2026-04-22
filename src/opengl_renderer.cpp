#include "sage/modules.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#if SAGE_ENABLE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

#include <array>
#include <iostream>

namespace sage {

class OpenGLRenderer final : public IRenderer {
public:
    bool initialize() override {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW.\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window_ = glfwCreateWindow(1280, 720, "Sage Engine", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create OpenGL window.\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        if (!gladLoadGL(glfwGetProcAddress)) {
            std::cerr << "Failed to load OpenGL functions via GLAD.\n";
            return false;
        }

#if SAGE_ENABLE_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
#endif

        return true;
    }

    void begin_frame() override {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if SAGE_ENABLE_IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        draw_debug_ui();
#endif
    }

    void end_frame() override {
#if SAGE_ENABLE_IMGUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    void shutdown() override {
#if SAGE_ENABLE_IMGUI
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif

        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }

    bool should_close() const override {
        return window_ == nullptr || glfwWindowShouldClose(window_);
    }

    void set_clear_color(const std::array<float, 4>& rgba) override {
        clear_color_ = rgba;
    }

private:
#if SAGE_ENABLE_IMGUI
    void draw_debug_ui() {
        ImGui::Begin("Sage Debug Panel");
        ImGui::Text("Renderer: OpenGL (GLFW + GLAD)");
        ImGui::ColorEdit4("Clear Color", clear_color_.data());
        ImGui::Text("Press ESC or close window to quit.");
        ImGui::End();

        ImGui::ShowDemoWindow();
    }
#endif

    GLFWwindow* window_{nullptr};
    std::array<float, 4> clear_color_{0.1f, 0.1f, 0.14f, 1.0f};
};

std::unique_ptr<IRenderer> make_opengl_renderer() {
    return std::make_unique<OpenGLRenderer>();
}

} // namespace sage
