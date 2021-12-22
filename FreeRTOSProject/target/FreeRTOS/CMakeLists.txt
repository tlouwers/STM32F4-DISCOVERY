# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# The project name (for the target)
project(freertos)

# Source files
add_library(${PROJECT_NAME} STATIC
    source/croutine.c
    source/event_groups.c
    source/heap_4.c
    source/list.c
    source/queue.c
    source/stream_buffer.c
    source/tasks.c
    source/timers.c
    portable/port.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portable
)

# Add compile options required by the library
target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)
