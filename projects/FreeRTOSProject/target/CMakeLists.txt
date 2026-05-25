
# Inspiration:
# https://dev.to/younup/cmake-on-stm32-the-beginning-3766


# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# The project name (for the target)
project(STM32F407-Discovery)

# Suppress spurious "CMAKE_TOOLCHAIN_FILE not used by project" warning — CMake
# processes the toolchain file internally before project(), so the project code
# never explicitly reads the variable. Touching it here marks it as used.
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "Toolchain file" FORCE)

# Enable assembler - needed to compile startup code file
enable_language(ASM)

# Path to the shared HAL/CMSIS at the repository root
set(HAL_ROOT "${CMAKE_SOURCE_DIR}/../../../hal")

# Path to the shared FreeRTOS at the repository root
set(FREERTOS_ROOT "${CMAKE_SOURCE_DIR}/../../../freertos")

# Source files - user, HAL and CMSIS
file(GLOB_RECURSE USER_SOURCES  "Src/*.c" "Src/*.cpp")
file(GLOB_RECURSE HAL_SOURCES   "${HAL_ROOT}/STM32F4xx_HAL_Driver/Src/*.c")
file(GLOB_RECURSE CMSIS_SOURCES "${HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Source/*.c")

# Include directories - FreeRTOS (SYSTEM: suppresses warnings from FreeRTOS headers)
include_directories(SYSTEM ${FREERTOS_ROOT}/include)

# Add other directories
add_subdirectory(FreeRTOS)

# HAL/CMSIS as an isolated static library — compiled without strict warnings.
# SYSTEM PUBLIC propagates HAL/CMSIS include paths to the executable as
# -isystem, so warnings from HAL headers are suppressed in user code too.
add_library(stm32_hal STATIC ${HAL_SOURCES} ${CMSIS_SOURCES})

target_compile_definitions(stm32_hal PUBLIC
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
)

target_include_directories(stm32_hal SYSTEM PUBLIC
    ${HAL_ROOT}/STM32F4xx_HAL_Driver/Inc
    ${HAL_ROOT}/CMSIS/Include
    ${HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Include
)

target_include_directories(stm32_hal PRIVATE
    ${CMAKE_SOURCE_DIR}/Src
)

target_compile_options(stm32_hal PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -fdata-sections
    -ffunction-sections

    -g3
)

# Canonical reusable drivers as an isolated static library — OUR code, so
# compiled with the strict warning set (definition lives in the shared
# drivers/drivers.cmake; see that file to add or change a driver).
include(${CMAKE_SOURCE_DIR}/../../../drivers/drivers.cmake)
stm32_add_drivers_library()

# Our executable (name)
set(EXECUTABLE ${PROJECT_NAME}.out)

add_executable(${EXECUTABLE}
    ${USER_SOURCES}
    ${CMAKE_SOURCE_DIR}/Startup/startup_stm32f407xx.s
)

target_include_directories(${EXECUTABLE} PRIVATE
    Src
)

# Add compile options required by the target
target_compile_options(${EXECUTABLE} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -fdata-sections
    -ffunction-sections
    -fstack-usage

    -Wall
    -g3

    # Extra warnings - catch common bugs at compile time
    -Wextra
    -Wshadow
    -Wdouble-promotion
    -Wundef
    -Wformat=2
    -Wcast-qual
    -Wcast-align
    -Wconversion
    -Wsign-conversion
    -Wlogical-op
    -Wnull-dereference
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Werror=return-type
    -Werror=implicit-fallthrough

    # Set specific C++ options
    $<$<COMPILE_LANGUAGE:CXX>:@${CMAKE_SOURCE_DIR}/gcc-options-cxx.txt>

    # When using Release, enable LinkTimeOptimizations
    $<$<CONFIG:Release>:-flto>
)

target_link_libraries(${EXECUTABLE} PRIVATE stm32_drivers stm32_hal freertos)

# Add link options required by the target
target_link_options(${EXECUTABLE} PRIVATE
    -T${CMAKE_SOURCE_DIR}/STM32F407VGTX_FLASH.ld

    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -specs=nano.specs
    -specs=nosys.specs

    -lc
    -lm
    -lnosys
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -Wl,--print-memory-usage

    # Linker hardening
    -Wl,--no-undefined
    -Wl,--warn-common

    # When using Release, enable Link Time Optimization
    $<$<CONFIG:Release>:-flto>
)

# Print executable size (post build step)
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD COMMAND arm-none-eabi-size ${EXECUTABLE})
