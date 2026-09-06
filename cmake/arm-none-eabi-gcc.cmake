# Shared arm-none-eabi cross-compile toolchain for every firmware project here.
#
# Single source of truth: the six projects previously each carried their own
# copy of this file. Five were byte-identical; the sixth (Bootloader) had
# silently drifted, missing the gcc-ar/gcc-ranlib wrappers that LTO needs when
# objects are archived into a static library.
#
# Each project keeps projects/<Name>/target/arm-none-eabi-gcc.cmake as a one-line
# include of this file, so every existing invocation still works unchanged --
# the .vscode/tasks.json entries, the READMEs, and CLAUDE.md all reference that
# per-project path.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

if(MINGW OR CYGWIN OR WIN32)
    set(UTIL_SEARCH_CMD where)
elseif(UNIX OR APPLE)
    set(UTIL_SEARCH_CMD which)
endif()

set(TOOLCHAIN_PREFIX arm-none-eabi-)

execute_process(
  COMMAND ${UTIL_SEARCH_CMD} ${TOOLCHAIN_PREFIX}gcc
  OUTPUT_VARIABLE BINUTILS_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objcopy  CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}size   CACHE INTERNAL "size tool")
set(CMAKE_AR        ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc-ar    CACHE INTERNAL "ar tool")
set(CMAKE_RANLIB    ${ARM_TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc-ranlib CACHE INTERNAL "ranlib tool")

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Override the 'Release' optimization flags (prevent using -O3)
set(CMAKE_C_FLAGS_RELEASE   "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")
