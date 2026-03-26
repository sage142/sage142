cmake_minimum_required(VERSION 3.20)
project(imgui_onnx_digit_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IMGUI_ONNX_FETCH_GLFW "Fetch GLFW from GitHub" ON)

if(NOT DEFINED ONNXRUNTIME_DIR AND DEFINED ENV{ONNXRUNTIME_DIR})
  set(ONNXRUNTIME_DIR "$ENV{ONNXRUNTIME_DIR}")
endif()

if(NOT ONNXRUNTIME_DIR)
  message(FATAL_ERROR "Please set ONNXRUNTIME_DIR to your ONNX Runtime installation directory.")
endif()

include(FetchContent)

if(IMGUI_ONNX_FETCH_GLFW)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
  )
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw)

if(APPLE)
  find_library(COCOA_FRAMEWORK Cocoa)
  find_library(IOKIT_FRAMEWORK IOKit)
  find_library(COREVIDEO_FRAMEWORK CoreVideo)
  find_library(OPENGL_FRAMEWORK OpenGL)
  target_link_libraries(imgui_lib PUBLIC ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK} ${OPENGL_FRAMEWORK})
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
endif()

add_executable(digit_demo
  src/main.cpp
)

target_include_directories(digit_demo PRIVATE
  ${ONNXRUNTIME_DIR}/include
)

if(WIN32)
  target_link_directories(digit_demo PRIVATE ${ONNXRUNTIME_DIR}/lib)
  target_link_libraries(digit_demo PRIVATE imgui_lib onnxruntime)
else()
  find_library(ONNXRUNTIME_LIB onnxruntime PATHS ${ONNXRUNTIME_DIR}/lib REQUIRED NO_DEFAULT_PATH)
  target_link_libraries(digit_demo PRIVATE imgui_lib ${ONNXRUNTIME_LIB} dl pthread)
endif()

if(MSVC)
  target_compile_options(digit_demo PRIVATE /W4)
else()
  target_compile_options(digit_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
