#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

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

void drawSquare(const glm::vec2& center, float size, const glm::vec3& color) {
    float half = size * 0.5f;

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f(center.x - half, center.y - half);
    glVertex2f(center.x + half, center.y - half);
    glVertex2f(center.x + half, center.y + half);
    glVertex2f(center.x - half, center.y + half);
    glEnd();
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return EXIT_FAILURE;
    }

    // Compatibility/OpenGL 2.1 context avoids requiring an extension loader (e.g., GLEW).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

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
    glfwSwapInterval(1);

    glViewport(0, 0, static_cast<int>(kWindowWidth), static_cast<int>(kWindowHeight));

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

        for (const Orb& orb : orbs) {
            if (orb.collected) {
                continue;
            }
            drawSquare(orb.position, 0.045f, orb.color);
        }

        drawSquare(player, 0.06f, glm::vec3(0.98f, 0.95f, 0.88f));

        glfwSetWindowTitle(window, buildHudText(score, static_cast<int>(orbs.size()), elapsed).c_str());

        if (elapsed >= kMaxTime) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Final score: " << score << "\n";

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
