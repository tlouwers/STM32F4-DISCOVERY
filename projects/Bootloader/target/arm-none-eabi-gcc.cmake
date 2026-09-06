# Cross-compile toolchain for this project.
#
# The real settings live in one shared file at the repository root so the six
# firmware projects cannot drift apart; see cmake/arm-none-eabi-gcc.cmake.
# This path is kept because .vscode/tasks.json, the README and CLAUDE.md all
# pass it as -DCMAKE_TOOLCHAIN_FILE.
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/arm-none-eabi-gcc.cmake)
