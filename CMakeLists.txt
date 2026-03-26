cmake_minimum_required(VERSION 3.16)
project(MathGenerationGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

add_executable(math_generation_game src/main.cpp)

target_link_libraries(math_generation_game
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        glm::glm
)

if (MSVC)
    target_compile_options(math_generation_game PRIVATE /W4)
else()
    target_compile_options(math_generation_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
