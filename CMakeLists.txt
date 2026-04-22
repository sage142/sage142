cmake_minimum_required(VERSION 3.21)
project(sage_engine VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAGE_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(SAGE_ENABLE_IMGUI "Enable Dear ImGui debug UI" ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Lua 5.4 REQUIRED)

add_executable(sage_engine
    src/main.cpp
    src/engine.cpp
    src/opengl_renderer.cpp
    src/lua_script_system.cpp
    src/publisher.cpp
)

target_include_directories(sage_engine
    PRIVATE
        include
        ${LUA_INCLUDE_DIR}
)

target_link_libraries(sage_engine
    PRIVATE
        OpenGL::GL
        glfw
        ${LUA_LIBRARIES}
)

if(SAGE_ENABLE_IMGUI)
    include(FetchContent)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.2b
    )
    FetchContent_MakeAvailable(imgui)

    target_sources(sage_engine PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
    )

    target_include_directories(sage_engine PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_compile_definitions(sage_engine PRIVATE SAGE_ENABLE_IMGUI=1)
else()
    target_compile_definitions(sage_engine PRIVATE SAGE_ENABLE_IMGUI=0)
endif()

if(SAGE_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(sage_engine PRIVATE /W4 /permissive-)
    else()
        target_compile_options(sage_engine PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_custom_target(run
    COMMAND sage_engine
    DEPENDS sage_engine
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Run sage_engine"
)
