# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# The project name (for the target)
project(freertos)

# Source files
add_library(${PROJECT_NAME} STATIC
    ${FREERTOS_ROOT}/source/croutine.c
    ${FREERTOS_ROOT}/source/event_groups.c
    ${FREERTOS_ROOT}/source/heap_4.c
    ${FREERTOS_ROOT}/source/list.c
    ${FREERTOS_ROOT}/source/queue.c
    ${FREERTOS_ROOT}/source/stream_buffer.c
    ${FREERTOS_ROOT}/source/tasks.c
    ${FREERTOS_ROOT}/source/timers.c
    ${FREERTOS_ROOT}/portable/port.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Src
    ${FREERTOS_ROOT}/include
    ${FREERTOS_ROOT}/portable
)

# Add compile options required by the library
target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)
