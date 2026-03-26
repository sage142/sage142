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

constexpr unsigned int kWindowWidth = 1200;
constexpr unsigned int kWindowHeight = 780;
constexpr float kPlayerSpeed = 4.0f;
constexpr float kSpawnRatePerSecond = 2.2f;
constexpr float kPickupRadius = 1.1f;
constexpr float kMaxTime = 90.0f;
constexpr float kPlayerY = 0.35f;

struct Orb {
    glm::vec3 position;
    glm::vec3 color;
    bool collected = false;
};

struct Projection {
    int width = static_cast<int>(kWindowWidth);
    int height = static_cast<int>(kWindowHeight);
} gProjection;

void setPerspective(float fovYDeg, float aspect, float zNear, float zFar) {
    const float fovYRad = fovYDeg * 3.1415926535f / 180.0f;
    const float top = zNear * std::tan(fovYRad * 0.5f);
    const float right = top * aspect;
    glFrustum(-right, right, -top, top, zNear, zFar);
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    gProjection.width = std::max(1, width);
    gProjection.height = std::max(1, height);
    glViewport(0, 0, gProjection.width, gProjection.height);
}

std::string buildHudText(int score, int generated, float elapsed) {
    std::ostringstream hud;
    hud << "3D Math Generation Garden | Score: " << score
        << " | Generated: " << generated
        << " | Time: " << std::max(0.0f, kMaxTime - elapsed);
    return hud.str();
}

Orb generateOrb(int index) {
    constexpr float goldenAngle = 2.39996323f;
    constexpr float radiusScale = 0.9f;

    const float n = static_cast<float>(index + 1);
    const float radius = radiusScale * std::sqrt(n);
    const float theta = n * goldenAngle;

    glm::vec3 pos{
        radius * std::cos(theta),
        0.55f + 0.45f * std::sin(n * 0.37f),
        radius * std::sin(theta)};

    glm::vec3 color{
        0.35f + 0.35f * std::sin(n * 0.17f),
        0.5f + 0.4f * std::cos(n * 0.11f + 1.2f),
        0.55f + 0.4f * std::sin(0.7f * n)};

    color = glm::clamp(color, glm::vec3(0.15f), glm::vec3(1.0f));
    return Orb{pos, color, false};
}

void drawCube(const glm::vec3& center, float size, const glm::vec3& color) {
    const float h = size * 0.5f;
    const float x = center.x;
    const float y = center.y;
    const float z = center.z;

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    // Front
    glVertex3f(x - h, y - h, z + h);
    glVertex3f(x + h, y - h, z + h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x - h, y + h, z + h);
    // Back
    glVertex3f(x - h, y - h, z - h);
    glVertex3f(x - h, y + h, z - h);
    glVertex3f(x + h, y + h, z - h);
    glVertex3f(x + h, y - h, z - h);
    // Left
    glVertex3f(x - h, y - h, z - h);
    glVertex3f(x - h, y - h, z + h);
    glVertex3f(x - h, y + h, z + h);
    glVertex3f(x - h, y + h, z - h);
    // Right
    glVertex3f(x + h, y - h, z - h);
    glVertex3f(x + h, y + h, z - h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x + h, y - h, z + h);
    // Top
    glVertex3f(x - h, y + h, z - h);
    glVertex3f(x - h, y + h, z + h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x + h, y + h, z - h);
    // Bottom
    glVertex3f(x - h, y - h, z - h);
    glVertex3f(x + h, y - h, z - h);
    glVertex3f(x + h, y - h, z + h);
    glVertex3f(x - h, y - h, z + h);
    glEnd();
}

void drawGround(float halfSize, int grid) {
    glColor3f(0.12f, 0.17f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(-halfSize, 0.0f, -halfSize);
    glVertex3f(halfSize, 0.0f, -halfSize);
    glVertex3f(halfSize, 0.0f, halfSize);
    glVertex3f(-halfSize, 0.0f, halfSize);
    glEnd();

    glColor3f(0.2f, 0.25f, 0.22f);
    glBegin(GL_LINES);
    for (int i = -grid; i <= grid; ++i) {
        const float x = static_cast<float>(i) * (halfSize / static_cast<float>(grid));
        glVertex3f(x, 0.001f, -halfSize);
        glVertex3f(x, 0.001f, halfSize);

        const float z = static_cast<float>(i) * (halfSize / static_cast<float>(grid));
        glVertex3f(-halfSize, 0.001f, z);
        glVertex3f(halfSize, 0.001f, z);
    }
    glEnd();
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(kWindowWidth),
        static_cast<int>(kWindowHeight),
        "3D Math Generation Garden", nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    std::vector<Orb> orbs;
    glm::vec3 player(0.0f, kPlayerY, 0.0f);
    int score = 0;

    float spawnAccumulator = 0.0f;
    float elapsed = 0.0f;
    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const float now = static_cast<float>(glfwGetTime());
        const float dt = now - lastFrame;
        lastFrame = now;
        elapsed += dt;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            move.z -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            move.z += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            move.x -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            move.x += 1.0f;
        }

        if (glm::length(move) > 0.001f) {
            player += glm::normalize(move) * kPlayerSpeed * dt;
        }

        player.x = std::clamp(player.x, -15.0f, 15.0f);
        player.z = std::clamp(player.z, -15.0f, 15.0f);

        spawnAccumulator += dt * kSpawnRatePerSecond;
        while (spawnAccumulator >= 1.0f) {
            orbs.push_back(generateOrb(static_cast<int>(orbs.size())));
            spawnAccumulator -= 1.0f;
        }

        for (Orb& orb : orbs) {
            if (orb.collected) {
                continue;
            }
            if (glm::distance(player, orb.position) <= kPickupRadius) {
                orb.collected = true;
                score += 1;
            }
        }

        glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        setPerspective(60.0f, static_cast<float>(gProjection.width) / static_cast<float>(gProjection.height), 0.1f, 120.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        const float cameraDistance = 9.5f;
        const glm::vec3 eye = player + glm::vec3(0.0f, 6.5f, cameraDistance);
        glTranslatef(-eye.x, -eye.y, -eye.z);

        drawGround(20.0f, 20);

        for (const Orb& orb : orbs) {
            if (!orb.collected) {
                drawCube(orb.position, 0.6f, orb.color);
            }
        }

        drawCube(player, 0.8f, glm::vec3(0.98f, 0.95f, 0.88f));

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
