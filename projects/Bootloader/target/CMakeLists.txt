# Firmware build for the Bootloader project (cross-compiled for ARM Cortex-M4).
#
# This is the umbrella for ALL device-side firmware. It builds two near-identical
# blink applications (blink_green, blink_orange) that share the bootloader-entry
# module and startup/linker/HAL configuration in 'shared/'. Configure and build
# this folder separately from the unit tests (see the project README).
#
# Inspiration: https://dev.to/younup/cmake-on-stm32-the-beginning-3766

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(BootloaderFirmware LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Suppress the spurious "CMAKE_TOOLCHAIN_FILE not used by project" warning — CMake
# processes the toolchain file before project(), so the variable is never read
# by project code. Touching it here marks it as used.
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "Toolchain file" FORCE)

# Path to the shared HAL/CMSIS at the repository root, and to this project's
# shared firmware (startup, linker, bootloader module, HAL config).
set(HAL_ROOT   "${CMAKE_SOURCE_DIR}/../../../hal")
set(SHARED_DIR "${CMAKE_SOURCE_DIR}/shared")

# ─────────────────────────────────────────────────────────────────────────────
# HAL/CMSIS as an isolated static library — third-party code, so compiled
# WITHOUT the strict warning set. SYSTEM PUBLIC propagates its include paths as
# -isystem so HAL header warnings do not leak into our code.
# ─────────────────────────────────────────────────────────────────────────────
file(GLOB_RECURSE HAL_SOURCES   "${HAL_ROOT}/STM32F4xx_HAL_Driver/Src/*.c")
file(GLOB_RECURSE CMSIS_SOURCES "${HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Source/*.c")

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

# stm32f4xx_hal_conf.h lives in shared/ and is needed to compile the HAL.
target_include_directories(stm32_hal PUBLIC ${SHARED_DIR})

target_compile_options(stm32_hal PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -fdata-sections
    -ffunction-sections
    -fstrict-volatile-bitfields

    -g3
)

# ─────────────────────────────────────────────────────────────────────────────
# Helper that builds one firmware application from <app>/Src plus the shared
# firmware sources. Invoked once per app from the app subdirectory.
# ─────────────────────────────────────────────────────────────────────────────
function(add_firmware_app APP_NAME)
    file(GLOB_RECURSE APP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp")
    # Drop CMake's in-tree compiler-probe sources so an in-source build/ dir does
    # not pull a second main() into the link.
    list(FILTER APP_SOURCES EXCLUDE REGEX "CMakeFiles/")

    set(EXECUTABLE ${APP_NAME}.elf)

    add_executable(${EXECUTABLE}
        ${APP_SOURCES}
        ${SHARED_DIR}/bootloader/BootloaderEntry.cpp
        ${SHARED_DIR}/bootloader/Stm32BootloaderHal.cpp
        ${SHARED_DIR}/stm32f4xx_it.c
        ${SHARED_DIR}/syscalls.c
        ${SHARED_DIR}/sysmem.c
        ${SHARED_DIR}/Startup/startup_stm32f407xx.s
    )

    # Output a plain "<app>.elf" (the name already carries the suffix).
    set_target_properties(${EXECUTABLE} PROPERTIES SUFFIX "")

    target_include_directories(${EXECUTABLE} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${SHARED_DIR}
    )

    # Our code — full strict warning set. Only return-type and implicit-fallthrough
    # are errors; the rest surface as warnings.
    target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard

        -fdata-sections
        -ffunction-sections
        -fstack-usage
        -fstrict-volatile-bitfields

        -Wall
        -g3

        -Wextra
        -Wpedantic
        -Wshadow
        -Wdouble-promotion
        -Wundef
        -Wformat=2
        -Wcast-qual
        -Wcast-align
        -Wconversion
        -Wsign-conversion
        -Wlogical-op
        -Wfloat-equal
        -Wlogical-not-parentheses
        -Wnull-dereference
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Werror=return-type
        -Werror=implicit-fallthrough

        # Specific C++ options (exceptions/RTTI off, etc.) live in a side file.
        $<$<COMPILE_LANGUAGE:CXX>:@${CMAKE_SOURCE_DIR}/gcc-options-cxx.txt>

        # When using Release, enable Link Time Optimization.
        $<$<CONFIG:Release>:-flto>
    )

    target_link_libraries(${EXECUTABLE} PRIVATE stm32_hal)

    target_link_options(${EXECUTABLE} PRIVATE
        -T${SHARED_DIR}/linker/stm32f4_app.ld

        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard

        -specs=nano.specs
        -specs=nosys.specs

        -lc
        -lm
        -lnosys
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.map,--cref
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,--warn-common

        $<$<CONFIG:Release>:-flto>
    )

    # Print binary size, then emit a raw .bin for flashing.
    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND arm-none-eabi-size ${EXECUTABLE})
    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${APP_NAME}.bin
        COMMENT "Generating ${APP_NAME}.bin")
endfunction()

add_subdirectory(blink_green)
add_subdirectory(blink_orange)
