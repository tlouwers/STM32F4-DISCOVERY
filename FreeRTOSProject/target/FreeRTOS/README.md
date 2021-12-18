# Overview
Sources were taken from FreeRTOS.org, the LTS release: (https://www.freertos.org/a00104.html).
From here files are taken from the 'FreeRTOS-Kernel' only.

# Reorganised
Files are reorganised for better maintainability:
* config: holds the 'FreeRTOSConfig.h' file with board specific configurations.
* license: holds the unmodified LICENSE and History file.
* port: holds the GCC\ARM_CM4F portable files, specific for the board.
* source: holds the source code files. Added to this is the 'MemMang' file 'heap_4.c'.
* source\include:  holds the include files, except 'stdint.readme'.

# Intent
A library is made of FreeRTOS, which is to be included in the main project.
It is configured for the STM32F4 board, with project specifics.
