#include "sage/modules.hpp"

#include <GLFW/glfw3.h>

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

        window_ = glfwCreateWindow(1280, 720, "Sage Engine", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create OpenGL window.\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        return true;
    }

    void begin_frame() override {
        glViewport(0, 0, 1280, 720);
        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void end_frame() override {
        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    void shutdown() override {
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
    GLFWwindow* window_{nullptr};
    std::array<float, 4> clear_color_{0.1f, 0.1f, 0.14f, 1.0f};
};

std::unique_ptr<IRenderer> make_opengl_renderer() {
    return std::make_unique<OpenGLRenderer>();
}

} // namespace sage
