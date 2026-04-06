cmake_minimum_required(VERSION 3.22)
project(bootloader_fw LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(HAL_ROOT    "${CMAKE_SOURCE_DIR}/../../../hal")
set(SHARED_DIR  "${CMAKE_SOURCE_DIR}/../shared")

file(GLOB_RECURSE HAL_SOURCES   "${HAL_ROOT}/STM32F4xx_HAL_Driver/Src/*.c")
file(GLOB_RECURSE CMSIS_SOURCES "${HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Source/*.c")

set(EXECUTABLE bootloader_fw)

add_executable(${EXECUTABLE}
    main.cpp
    ${SHARED_DIR}/Startup/startup_stm32f407xx.s
    ${HAL_SOURCES}
    ${CMSIS_SOURCES}
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${HAL_ROOT}/STM32F4xx_HAL_Driver/Inc
    ${HAL_ROOT}/CMSIS/Include
    ${HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Include
    ${SHARED_DIR}
)

target_compile_definitions(${EXECUTABLE} PRIVATE
    USE_HAL_DRIVER
    STM32F407xx
)

target_compile_options(${EXECUTABLE} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -fdata-sections
    -ffunction-sections
    -Wall
    -g3
    $<$<CONFIG:Release>:-Os>
)

set_target_properties(${EXECUTABLE} PROPERTIES SUFFIX ".elf")

target_link_options(${EXECUTABLE} PRIVATE
    -T${SHARED_DIR}/linker/stm32f4_bootloader.ld
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}.map,--cref
    -Wl,--gc-sections
    -specs=nosys.specs
    -specs=nano.specs
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin
    COMMENT "Generating ${EXECUTABLE}.bin"
)
