# drivers.cmake — shared definition of the canonical driver library.
#
# This file is the single source of truth for building the reusable
# drivers/ tree into a project's firmware. Every project's
# target/CMakeLists.txt does:
#
#     include(${CMAKE_SOURCE_DIR}/../../../drivers/drivers.cmake)
#     stm32_add_drivers_library()
#     ...
#     target_link_libraries(${EXECUTABLE} PRIVATE stm32_drivers stm32_hal)
#
# Design:
#  * One STATIC library `stm32_drivers` from the canonical
#    drivers/{drivers,components,utility} sources. board/ is NOT included
#    — BoardConfig pin maps are project-specific and stay under Src/board.
#  * It is OUR code, so it is compiled with the strict warning set
#    (mirrors the executable), unlike `stm32_hal` which is relaxed.
#  * Built per project (PRIVATE-includes that project's Src/ for its
#    config.h / stm32f4xx_hal_conf.h) but the source list lives here, so
#    adding/changing a driver is a one-place change for all projects.
#  * Requires the `stm32_hal` target to already be defined by the caller.
#
# Adding a driver: drop it under drivers/{drivers,components,utility}/ and
# re-run cmake (sources are globbed — a fresh configure is needed because
# CMake globs are evaluated at configure time).

# Resolve the canonical drivers/ root from this file's own location,
# captured at include() time (CMAKE_CURRENT_LIST_DIR changes inside a
# function called from another file, so it must be read here).
set(STM32_DRIVERS_ROOT "${CMAKE_CURRENT_LIST_DIR}")

function(stm32_add_drivers_library)
    if(NOT TARGET stm32_hal)
        message(FATAL_ERROR
            "stm32_add_drivers_library(): the 'stm32_hal' target must be "
            "defined before this is called.")
    endif()

    file(GLOB_RECURSE STM32_DRIVERS_SOURCES
        "${STM32_DRIVERS_ROOT}/drivers/*.cpp"
        "${STM32_DRIVERS_ROOT}/components/*.cpp"
        "${STM32_DRIVERS_ROOT}/utility/*.cpp"
        "${STM32_DRIVERS_ROOT}/utility/*.c"
    )

    add_library(stm32_drivers STATIC ${STM32_DRIVERS_SOURCES})

    # Canonical tree root is the include base: this dir holds
    # drivers/ interfaces/ components/ utility/, so "drivers/Adc/Adc.hpp",
    # "interfaces/IAdc.hpp", "components/..", "utility/.." all resolve.
    # PUBLIC so the executable that links this library inherits the path.
    target_include_directories(stm32_drivers PUBLIC
        ${STM32_DRIVERS_ROOT}
    )

    # The drivers pull in the project's HAL config and config.h.
    target_include_directories(stm32_drivers PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
    )

    # Drivers include "stm32f4xx_hal.h" etc.; stm32_hal carries those as
    # SYSTEM include dirs, so HAL header warnings stay suppressed here too.
    target_link_libraries(stm32_drivers PUBLIC stm32_hal)

    # Our code → strict warnings (kept in sync with the executable's set).
    target_compile_options(stm32_drivers PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard

        -fdata-sections
        -ffunction-sections
        -fstrict-volatile-bitfields

        -Wall
        -g3

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

        $<$<COMPILE_LANGUAGE:CXX>:@${CMAKE_SOURCE_DIR}/gcc-options-cxx.txt>

        $<$<CONFIG:Release>:-flto>
    )
endfunction()
