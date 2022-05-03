
# Inspiration:
# https://dev.to/younup/cmake-on-stm32-the-beginning-3766


# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# The project name (for the target)
project(STM32F407-Discovery)

# Enable assembler - needed to compile startup code file
enable_language(ASM)

# Source files - user, HAL and CMSIS
file(GLOB_RECURSE USER_SOURCES  "Src/*.c" "Src/*.cpp")
file(GLOB_RECURSE HAL_SOURCES   "Drivers/STM32F4xx_HAL_Driver/Src/*.c")
file(GLOB_RECURSE CMSIS_SOURCES "Drivers/CMSIS/Device/ST/STM32F4xx/Source/*.c")

# Include directories - user, HAL and CMSIS
include_directories(Src)
include_directories(Drivers/STM32F4xx_HAL_Driver/Inc)
include_directories(Drivers/CMSIS/Include)
include_directories(Drivers/CMSIS/Device/ST/STM32F4xx/Include)

# Include directories - FreeRTOS
include_directories(FreeRTOS/include)

# Add other directories
add_subdirectory(FreeRTOS)

# Our executable (name)
set(EXECUTABLE ${PROJECT_NAME}.out)

# Add the sources which make up the executable
add_executable(${EXECUTABLE} ${USER_SOURCES} ${HAL_SOURCES} ${CMSIS_SOURCES} ${CMAKE_SOURCE_DIR}/Startup/startup_stm32f407xx.s)

# Link in the FreeRTOS library
target_link_libraries(${PROJECT_NAME}.out freertos)

# Add compile flags required by the HAL
target_compile_definitions(${EXECUTABLE} PRIVATE
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
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

    # Set specific C++ options
    $<$<COMPILE_LANGUAGE:CXX>:@${CMAKE_SOURCE_DIR}/gcc-options-cxx.txt>

    # When using Release, enable LinkTimeOptimizations
    $<$<CONFIG:Release>:-flto>
)

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

    # When using Release, enable Link Time Optimization
    $<$<CONFIG:Release>:-flto>
)

# Print executable size (post build step)
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD COMMAND arm-none-eabi-size ${EXECUTABLE})
