#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr unsigned int kWindowWidth = 1100;
constexpr unsigned int kWindowHeight = 720;
constexpr float kPlayerSpeed = 0.95f;
constexpr float kSpawnRatePerSecond = 3.0f;
constexpr float kPickupRadius = 0.072f;
constexpr float kMaxTime = 90.0f;
constexpr float kBackgroundFade = 0.08f;

struct Orb {
    glm::vec2 position;
    glm::vec3 color;
    bool collected = false;
};

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

std::string buildHudText(int score, int generated, float elapsed) {
    std::ostringstream hud;
    hud << "Math Generation Garden | Score: " << score
        << " | Generated: " << generated
        << " | Time: " << std::max(0.0f, kMaxTime - elapsed);
    return hud.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader compile error: " << log << '\n';
        std::exit(EXIT_FAILURE);
    }

    return shader;
}

GLuint createProgram() {
    const char* vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;

        uniform vec2 uTranslation;
        uniform vec2 uScale;

        void main() {
            vec2 scaled = aPos * uScale;
            gl_Position = vec4(scaled + uTranslation, 0.0, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;

        uniform vec3 uColor;

        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )";

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<size_t>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::cerr << "Program link error: " << log << '\n';
        std::exit(EXIT_FAILURE);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

Orb generateOrb(int index) {
    constexpr float goldenAngle = 2.39996323f;
    constexpr float radiusScale = 0.085f;

    float n = static_cast<float>(index + 1);
    float r = radiusScale * std::sqrt(n);
    float theta = n * goldenAngle;

    glm::vec2 pos{
        r * std::cos(theta),
        r * std::sin(theta)
    };

    float wave = std::sin(0.7f * n);
    glm::vec3 color{
        0.35f + 0.35f * std::sin(n * 0.17f),
        0.5f + 0.4f * std::cos(n * 0.11f + 1.2f),
        0.55f + 0.4f * wave
    };

    color = glm::clamp(color, glm::vec3(0.15f), glm::vec3(1.0f));
    return Orb{pos, color, false};
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(kWindowWidth),
        static_cast<int>(kWindowHeight),
        "Math Generation Garden", nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint program = createProgram();
    glUseProgram(program);

    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    GLint translationLoc = glGetUniformLocation(program, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(program, "uScale");
    GLint colorLoc = glGetUniformLocation(program, "uColor");

    glfwSwapInterval(1);

    std::vector<Orb> orbs;
    glm::vec2 player(0.0f, 0.0f);
    int score = 0;

    float spawnAccumulator = 0.0f;
    float elapsed = 0.0f;
    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float now = static_cast<float>(glfwGetTime());
        float dt = now - lastFrame;
        lastFrame = now;
        elapsed += dt;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glm::vec2 axis(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            axis.y += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            axis.y -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            axis.x -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            axis.x += 1.0f;
        }

        if (glm::length(axis) > 0.001f) {
            player += glm::normalize(axis) * kPlayerSpeed * dt;
        }
        player = glm::clamp(player, glm::vec2(-0.97f), glm::vec2(0.97f));

        spawnAccumulator += dt * kSpawnRatePerSecond;
        while (spawnAccumulator >= 1.0f) {
            orbs.push_back(generateOrb(static_cast<int>(orbs.size())));
            spawnAccumulator -= 1.0f;
        }

        for (Orb& orb : orbs) {
            if (orb.collected) {
                continue;
            }
            float dist = glm::distance(player, orb.position);
            if (dist <= kPickupRadius) {
                orb.collected = true;
                score += 1;
            }
        }

        glClearColor(kBackgroundFade, kBackgroundFade, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);

        for (const Orb& orb : orbs) {
            if (orb.collected) {
                continue;
            }
            glUniform2f(translationLoc, orb.position.x, orb.position.y);
            glUniform2f(scaleLoc, 0.045f, 0.045f);
            glUniform3f(colorLoc, orb.color.r, orb.color.g, orb.color.b);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glUniform2f(translationLoc, player.x, player.y);
        glUniform2f(scaleLoc, 0.06f, 0.06f);
        glUniform3f(colorLoc, 0.98f, 0.95f, 0.88f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSetWindowTitle(window, buildHudText(score, static_cast<int>(orbs.size()), elapsed).c_str());

        if (elapsed >= kMaxTime) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Final score: " << score << "\n";

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
