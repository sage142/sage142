cmake_minimum_required(VERSION 3.16)
project(RelocatePlumber LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(relocate_plumber
    src/main.cpp
)

target_include_directories(relocate_plumber PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(relocate_plumber PRIVATE ${SDL2_LIBRARIES})

if (MSVC)
    target_compile_options(relocate_plumber PRIVATE /W4)
else()
    target_compile_options(relocate_plumber PRIVATE -Wall -Wextra -Wpedantic)
endif()
