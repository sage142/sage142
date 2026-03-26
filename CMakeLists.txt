cmake_minimum_required(VERSION 3.16)
project(MathGenerationGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(MATH_GAME_USE_SYSTEM_DEPS "Prefer system-installed GLFW/GLEW/GLM" ON)

find_package(OpenGL REQUIRED)

set(USE_BUNDLED_GLEW OFF)

if(MATH_GAME_USE_SYSTEM_DEPS)
    find_package(glfw3 QUIET)
    find_package(GLEW QUIET)
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

if(NOT TARGET GLEW::GLEW)
    message(STATUS "GLEW package not found. Fetching GLEW source from GitHub...")
    FetchContent_Declare(
        glew_src
        GIT_REPOSITORY https://github.com/nigels-com/glew.git
        GIT_TAG glew-2.2.0
    )
    FetchContent_MakeAvailable(glew_src)

    add_library(glew_static STATIC ${glew_src_SOURCE_DIR}/src/glew.c)
    target_include_directories(glew_static PUBLIC ${glew_src_SOURCE_DIR}/include)
    target_link_libraries(glew_static PUBLIC OpenGL::GL)
    add_library(GLEW::GLEW ALIAS glew_static)
    set(USE_BUNDLED_GLEW ON)
endif()

add_executable(math_generation_game src/main.cpp)

target_link_libraries(math_generation_game
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        glm::glm
)

if(USE_BUNDLED_GLEW)
    target_compile_definitions(math_generation_game PRIVATE GLEW_STATIC)
endif()

if (MSVC)
    target_compile_options(math_generation_game PRIVATE /W4)
else()
    target_compile_options(math_generation_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
