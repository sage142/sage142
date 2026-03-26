cmake_minimum_required(VERSION 3.16)
project(MathGenerationGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(MATH_GAME_USE_SYSTEM_DEPS "Prefer system-installed GLFW/GLM" ON)

find_package(OpenGL REQUIRED)

if(MATH_GAME_USE_SYSTEM_DEPS)
    find_package(glfw3 QUIET)
    find_package(glm QUIET)
endif()

if(NOT TARGET glfw)
    message(STATUS "glfw3 package not found. Fetching GLFW from GitHub...")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
endif()

if(NOT TARGET glm::glm)
    message(STATUS "glm package not found. Fetching GLM from GitHub...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

add_executable(math_generation_game src/main.cpp)

target_link_libraries(math_generation_game
    PRIVATE
        OpenGL::GL
        glfw
        glm::glm
)

if (MSVC)
    target_compile_options(math_generation_game PRIVATE /W4)
else()
    target_compile_options(math_generation_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
