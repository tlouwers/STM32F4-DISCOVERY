# SVD File
This file contains a description of the processor registers, their location, size and how to interpret them. The [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension for Visual Studio Code depends on this to provide additional debug information.

## Download location
[STM32F4 boards SVD files](https://www.st.com/resource/en/svd/stm32f4_svd.zip)
Unpack the zipfile, use file STM32L0x3.svd for the project since we have a STM32 F407VG chip on the Discovery board.

## Configuration
The file is placed in the folder: `3rd-party\svd`, the configuration for the Cortex-Debug plugin should be extended to include this file (launch.json snippit):
```cpp
{
	"name": "Debug node (OpenOCD)",
	"cwd": "${workspaceFolder}/target",
	"executable": "build/STM32F407-Discovery.out",
	"request": "launch",
	"type": "cortex-debug",
	"servertype": "openocd",
	"device": "STM32F407VG",
	"svdFile": "${workspaceFolder}/3rd-party/svd/STM32F407.svd",
	"interface": "swd",
	"configFiles": [
		"board/stm32f4discovery.cfg"
	],
	"runToMain": true,
},
```
