cmake_minimum_required(VERSION 3.21)
project(sage_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAGE_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

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
