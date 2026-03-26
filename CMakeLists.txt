cmake_minimum_required(VERSION 3.21)
project(BeatCanvasDAW LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(LAME QUIET lame)
endif()

# Try several ways to get GLFW:
# 1) Config package (glfw3Config.cmake)
# 2) CMake module package (glfw)
# 3) pkg-config fallback
find_package(glfw3 CONFIG QUIET)
if (NOT TARGET glfw)
  find_package(glfw QUIET)
endif()

if (NOT TARGET glfw)
  if (PkgConfig_FOUND)
    pkg_check_modules(GLFW3 QUIET glfw3)
  endif()
  if (GLFW3_FOUND)
    add_library(glfw INTERFACE IMPORTED)
    target_include_directories(glfw INTERFACE ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(glfw INTERFACE ${GLFW3_LIBRARIES})
    target_link_directories(glfw INTERFACE ${GLFW3_LIBRARY_DIRS})
  else()
    message(FATAL_ERROR "GLFW not found. Install glfw3 dev package or set CMAKE_PREFIX_PATH/glfw3_DIR.")
  endif()
endif()

# Dear ImGui is expected as a local subfolder for offline-friendly setup.
# Put Dear ImGui source at: third_party/imgui/
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
if (NOT EXISTS "${IMGUI_DIR}/imgui.h")
  message(FATAL_ERROR "Dear ImGui not found. Please place it in third_party/imgui")
endif()

add_library(imgui_lib
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

add_executable(beat_canvas src/main.cpp)

target_link_libraries(beat_canvas PRIVATE imgui_lib)

if (LAME_FOUND)
  target_compile_definitions(beat_canvas PRIVATE DAW_HAS_LAME=1)
  target_include_directories(beat_canvas PRIVATE ${LAME_INCLUDE_DIRS})
  target_link_libraries(beat_canvas PRIVATE ${LAME_LIBRARIES})
else()
  message(WARNING "lame not found - MP3 export button will show an explanatory message.")
endif()

if (MSVC)
  target_compile_options(beat_canvas PRIVATE /W4)
else()
  target_compile_options(beat_canvas PRIVATE -Wall -Wextra -Wpedantic)
endif()
