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

find_package(glad CONFIG QUIET)
set(GLAD_TARGET "")
if(TARGET glad::glad)
    set(GLAD_TARGET glad::glad)
elseif(TARGET glad)
    set(GLAD_TARGET glad)
else()
    message(WARNING "GLAD package not found. Building without GLAD loader. Install glad (e.g. vcpkg install glad) to enable it.")
endif()

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)

target_include_directories(imgui
    PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_executable(math_generation_game src/main.cpp)

target_link_libraries(math_generation_game
    PRIVATE
        OpenGL::GL
        glfw
        glm::glm
        imgui
)

if(GLAD_TARGET)
    target_link_libraries(math_generation_game PRIVATE ${GLAD_TARGET})
    target_compile_definitions(math_generation_game PRIVATE MATH_GAME_USE_GLAD=1)
else()
    target_compile_definitions(math_generation_game PRIVATE MATH_GAME_USE_GLAD=0)
endif()

if (MSVC)
    target_compile_options(math_generation_game PRIVATE /W4)
else()
    target_compile_options(math_generation_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
