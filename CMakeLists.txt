cmake_minimum_required(VERSION 3.21)
project(sage_engine VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAGE_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(SAGE_ENABLE_IMGUI "Enable Dear ImGui debug UI" ON)
option(SAGE_FETCH_DEPS "Fetch missing third-party dependencies automatically" ON)

include(FetchContent)

find_package(OpenGL REQUIRED)
find_package(Lua 5.4 REQUIRED)

set(SAGE_GLFW_TARGET "")
find_package(glfw3 QUIET)
if(TARGET glfw)
    set(SAGE_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(SAGE_GLFW_TARGET glfw3::glfw)
elseif(SAGE_FETCH_DEPS)
    message(STATUS "glfw3 not found; fetching GLFW from source")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
    set(SAGE_GLFW_TARGET glfw)
else()
    message(FATAL_ERROR
        "glfw3 was not found. Install glfw3 (dev package), set glfw3_DIR/CMAKE_PREFIX_PATH, "
        "or enable SAGE_FETCH_DEPS to fetch it automatically.")
endif()

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
        ${SAGE_GLFW_TARGET}
        ${LUA_LIBRARIES}
)

if(SAGE_ENABLE_IMGUI)
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
