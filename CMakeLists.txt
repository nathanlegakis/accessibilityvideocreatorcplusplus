cmake_minimum_required(VERSION 3.20)
project(accessibility_video_creator_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(accessibility_video_creator
    src/main.cpp
    external/sqlite/sqlite3.c
)

target_include_directories(accessibility_video_creator PRIVATE
    external/sqlite
)

target_compile_options(accessibility_video_creator PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if (WIN32)
    target_link_libraries(accessibility_video_creator PRIVATE sapi ole32)
endif()

# Future: sherpa-onnx integration
# Option A: add_subdirectory(external/sherpa-onnx)
# Option B: find_package(sherpa-onnx CONFIG REQUIRED)
