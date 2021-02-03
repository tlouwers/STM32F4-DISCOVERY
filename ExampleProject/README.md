# Description
A basic example or project structure for C++ development with the STM32F407G-DISC1 kit.

Intended use is to be a starting point for developing, debugging and unit testing the STM32F407G-DISC1.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++14 is assumed
* MingW-W64 as GCC compiler for Windows (https://sourceforge.net/projects/mingw-w64/)
* ARM Embedded as GCC compiler for Target (STM32F4) (https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). Note: remember to add these to PATH variable. No, really: something like: 'C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2020-q4-major\binC:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2020-q4-major\bin'. And log off, then on again, or reboot to make these active.
* Git. Have Git installed as wel to be able to retrieve the Google Test framework (https://git-scm.com/download/win)
* Ninja as build system (https://ninja-build.org/)
* The Cortex-Debug extension together with the proper device support pack to be able to debug and view registers of the target (https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
* OpenOCD to to connect with the target (https://gnutoolchains.com/arm-eabi/openocd/)

# Features
* CMake setup for building STM32F4 code
* Can configure and build, even startup assembler code
* Can build, run and debug unit tests on Windows (example available)
* Can build, deplay and debug on target using OpenOCD and Cortex-Debug
* Release builds always have Link Time Optimization enabled, as well as including debug symbols
* Some statistics for the generated target binary are displayed after each build
* Configuration of the cross-compile settings in a seperate CMake file
* All C++ compiler flags in a seperate configuration file

# Notes
* In 'Extensions', click the 'CMake Tools'extension. Then click the cog ('manage'), then 'Extension Settings'. This opens the settings of the extension. Scroll down to 'CMake: Generator', in the textbox below enter: 'Ninja'. This is the generator used per default for our project.
* Use either 'Debug' or 'Release' for configuring (and building) the target.
* A 'Release' build is modified to be '-Os' instead of something else in the 'arm-none-eabi-gcc.cmake' file.
* A 'Release' build always uses Link Time Optimization to produce a smaller binary. This is not displayed in the terminal - you can see it in the binary statistics only.
* Unit tests only have Debug build.

# Overview
* 3rd-party\googletest - This folder contains the Google Test framework. It will be downloaded and updated automatically if Git is installed.
* target - This folder contains the Cortex-M4 (ARM) project to be build (Cross-Compiled). The 'build' folder contains the artifacts created, the 'Drivers' folder contains the CMSIS and STM32 HAL code. The 'Src' folder contains the user code, drivers and application logic. The 'Startup' folder contains the startup assembly file. The 'arm-none-eabi-gcc.cmake' file contains setting to Cross-Compile for ARM. The file 'gcc-options-cxx.txt' contains specific GCC flags. They are present here as not to clutter the CMake file.
* tests - This folder contains the unit tests as available for the project. It is build for PC (not ARM) and uses code from the 'target' to test.

# Usage
* Set the toolkit (compiler) to the Mingw64 installed one.
* Use the 'Terminal -> Run Task' menu to configure and build either the target or the tests.
* Use the 'Run -> '.
* The build for for the unit tests is in the 'Project1' folder, the build folder for the target is in the 'target' folder.
* Per default for the target the binary (*.out) and mapfile (*.map) are generated.
* It may be needed to delete the 'build' folder inside the 'Project1' folder to be able to configure and build the target. It may be needed to close Visual Studio Code before Windows 10 allows it.

# GCC options C++
Specific C++ compiler flags can be changed by altering the 'gcc-options-cxx.txt' file. Please do not add them to the CMakeLists.txt file.
| Compiler Flag | Description |
| --- | --- |
| -fno-exceptions | Disable exception handling |
| -fno-non-call-exceptions | Disable generation of code that allows trapping instructions to throw exceptions |
| -fno-unwind-tables | Disable generation of any needed static data by exceptions |
| -fno-rtti | Do not allow usage of the typeid operator as well as dynamic_cast |
| -fno-threadsafe-statics | Do not emit the extra code to use the routines specified in the C++ ABI for thread-safe initialization of local statics |
| -fno-use-cxa-atexit | Do not register destructors for objects with static storage duration with the __cxa_atexit function rather than the atexit function |
| -fno-common | Place uninitialized global variables in the BSS section of the object file |
| -fmessage-length=0 | Format error messages to be appear on a single line |

# Configuration / Future Updates
There will be a time components are updates, meaning this example and environment needs to be updated as well. As reminder (for me):
* In folder '.vscode'
- launch.json - contains the settings to start debugging, either the target (using OpenOCD) or the unit test executable.
- settings.json - contains the settings which make IntelliSense happy, compiler and folder dependent.
- tasks.json - contains the settings for configuration and building the project, both the target and unit tests.
* In folder 'target/Drivers/STM32F4xx_HAL_Driver' is the STM32F4 HAL code.
* The file 'arm-none-eabi-gg.cmake' contains the settings required for cross compiling for ARM.
