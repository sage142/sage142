#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl2.h>

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
constexpr float kVerticalSpeed = 3.0f;
constexpr float kMaxTime = 90.0f;

struct GenerationConfig {
    float spawnRatePerSecond = 2.2f;
    float pickupRadius = 1.1f;
    float radiusScale = 0.9f;
    float heightBase = 0.55f;
    float heightAmplitude = 0.45f;
    float heightFrequency = 0.37f;
    float colorFrequency = 0.17f;
};

struct CameraConfig {
    float yawDeg = 0.0f;
    float pitchDeg = 28.0f;
    float distance = 10.0f;
    float heightOffset = 4.5f;
};

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

Orb generateOrb(int index, const GenerationConfig& config) {
    constexpr float goldenAngle = 2.39996323f;

    const float n = static_cast<float>(index + 1);
    const float radius = config.radiusScale * std::sqrt(n);
    const float theta = n * goldenAngle;

    glm::vec3 pos{
        radius * std::cos(theta),
        config.heightBase + config.heightAmplitude * std::sin(n * config.heightFrequency),
        radius * std::sin(theta)};

    glm::vec3 color{
        0.35f + 0.35f * std::sin(n * config.colorFrequency),
        0.5f + 0.4f * std::cos(n * (config.colorFrequency * 0.65f) + 1.2f),
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
    glVertex3f(x - h, y - h, z + h);
    glVertex3f(x + h, y - h, z + h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x - h, y + h, z + h);
    glVertex3f(x - h, y - h, z - h);
    glVertex3f(x - h, y + h, z - h);
    glVertex3f(x + h, y + h, z - h);
    glVertex3f(x + h, y - h, z - h);
    glVertex3f(x - h, y - h, z - h);
    glVertex3f(x - h, y - h, z + h);
    glVertex3f(x - h, y + h, z + h);
    glVertex3f(x - h, y + h, z - h);
    glVertex3f(x + h, y - h, z - h);
    glVertex3f(x + h, y + h, z - h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x + h, y - h, z + h);
    glVertex3f(x - h, y + h, z - h);
    glVertex3f(x - h, y + h, z + h);
    glVertex3f(x + h, y + h, z + h);
    glVertex3f(x + h, y + h, z - h);
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

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    GenerationConfig config;
    CameraConfig camera;
    std::vector<Orb> orbs;
    glm::vec3 player(0.0f, 0.35f, 0.0f);

    int score = 0;
    float spawnAccumulator = 0.0f;
    float elapsed = 0.0f;
    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const float now = static_cast<float>(glfwGetTime());
        const float dt = now - lastFrame;
        lastFrame = now;
        elapsed += dt;

        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Generation Controls");
        ImGui::Text("Tune generation + camera");
        ImGui::SliderFloat("Spawn Rate", &config.spawnRatePerSecond, 0.2f, 8.0f, "%.2f / s");
        ImGui::SliderFloat("Pickup Radius", &config.pickupRadius, 0.2f, 3.0f, "%.2f");
        ImGui::SliderFloat("Radius Scale", &config.radiusScale, 0.2f, 2.0f, "%.2f");
        ImGui::SliderFloat("Height Base", &config.heightBase, 0.1f, 2.5f, "%.2f");
        ImGui::SliderFloat("Height Amp", &config.heightAmplitude, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Height Freq", &config.heightFrequency, 0.05f, 1.5f, "%.2f");
        ImGui::SliderFloat("Color Freq", &config.colorFrequency, 0.05f, 0.8f, "%.2f");

        ImGui::Separator();
        ImGui::SliderFloat("Camera Yaw", &camera.yawDeg, -180.0f, 180.0f, "%.1f deg");
        ImGui::SliderFloat("Camera Pitch", &camera.pitchDeg, 5.0f, 85.0f, "%.1f deg");
        ImGui::SliderFloat("Camera Distance", &camera.distance, 3.0f, 22.0f, "%.1f");
        ImGui::SliderFloat("Camera Height", &camera.heightOffset, 0.0f, 12.0f, "%.1f");

        if (ImGui::Button("Clear Orbs")) {
            orbs.clear();
            score = 0;
            spawnAccumulator = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Player")) {
            player = glm::vec3(0.0f, 0.35f, 0.0f);
        }

        ImGui::Text("Move: WASD/Arrows, Up: Space, Down: Left Shift, Camera: Q/E + R/F");
        ImGui::Text("Orbs: %d", static_cast<int>(orbs.size()));
        ImGui::Text("Collected: %d", score);
        ImGui::End();

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
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            move.y += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            move.y -= 1.0f;
        }

        if (glm::length(move) > 0.001f) {
            glm::vec3 horizontal(move.x, 0.0f, move.z);
            if (glm::length(horizontal) > 0.001f) {
                horizontal = glm::normalize(horizontal) * kPlayerSpeed * dt;
                player.x += horizontal.x;
                player.z += horizontal.z;
            }
            player.y += move.y * kVerticalSpeed * dt;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.yawDeg -= 65.0f * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.yawDeg += 65.0f * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            camera.pitchDeg = std::min(85.0f, camera.pitchDeg + 45.0f * dt);
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            camera.pitchDeg = std::max(5.0f, camera.pitchDeg - 45.0f * dt);
        }

        player.x = std::clamp(player.x, -15.0f, 15.0f);
        player.z = std::clamp(player.z, -15.0f, 15.0f);
        player.y = std::clamp(player.y, 0.2f, 8.0f);

        spawnAccumulator += dt * config.spawnRatePerSecond;
        while (spawnAccumulator >= 1.0f) {
            orbs.push_back(generateOrb(static_cast<int>(orbs.size()), config));
            spawnAccumulator -= 1.0f;
        }

        for (Orb& orb : orbs) {
            if (orb.collected) {
                continue;
            }
            if (glm::distance(player, orb.position) <= config.pickupRadius) {
                orb.collected = true;
                score += 1;
            }
        }

        glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        setPerspective(60.0f, static_cast<float>(gProjection.width) / static_cast<float>(gProjection.height), 0.1f, 120.0f);

        const float yawRad = glm::radians(camera.yawDeg);
        const float pitchRad = glm::radians(camera.pitchDeg);
        const glm::vec3 forward(
            std::sin(yawRad) * std::cos(pitchRad),
            -std::sin(pitchRad),
            -std::cos(yawRad) * std::cos(pitchRad));

        const glm::vec3 cameraTarget = player + glm::vec3(0.0f, camera.heightOffset * 0.2f, 0.0f);
        const glm::vec3 eye = cameraTarget - forward * camera.distance + glm::vec3(0.0f, camera.heightOffset, 0.0f);
        const glm::mat4 view = glm::lookAt(eye, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(view));

        drawGround(20.0f, 20);
        for (const Orb& orb : orbs) {
            if (!orb.collected) {
                drawCube(orb.position, 0.6f, orb.color);
            }
        }
        drawCube(player, 0.8f, glm::vec3(0.98f, 0.95f, 0.88f));

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSetWindowTitle(window, buildHudText(score, static_cast<int>(orbs.size()), elapsed).c_str());
        if (elapsed >= kMaxTime) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwSwapBuffers(window);
    }

    std::cout << "Final score: " << score << "\n";

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
