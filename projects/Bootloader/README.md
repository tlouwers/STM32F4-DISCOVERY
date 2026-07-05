# Description

A UART bootloader workflow for the STM32F407G-DISC1 board, built on top of the
**ST system memory bootloader** (the factory ROM at `0x1FFF0000`). Rather than
writing a custom device-side bootloader, this project drives the proven ST
bootloader over its AN3155 USART protocol and puts the engineering effort into a
robust, branded **host tool** plus a tiny device-side entry module.

The deliverables are:

* A **.NET 8 host application** (`host/`) implementing the AN3155 protocol and a
  full factory-reset orchestrator (connect → erase → write → verify → boot) with
  CRC-mismatch retry and fail-fast handling of mid-transfer connection loss.
  It ships both a **CLI** and a guided **Avalonia GUI** that auto-discovers the
  USB-UART cable, auto-probes for a live bootloader, and only enables *Flash*
  once a device is connected and a valid image is loaded.
* A minimal **`BootloaderEntry` module** added to the application firmware
  (`target/shared/bootloader/`) that lets a running app request a jump into the
  ST bootloader (write a magic value to an RTC backup register, then reset).
* Two **sample blink applications** (`target/blink_green`, `target/blink_orange`)
  used to demonstrate and validate the full update cycle.

See `docs/plan.md` for the full design, protocol notes, and phased plan.

# Quick start

End-to-end update cycle on real hardware (host-free entry via the blue button):

1. **Build + flash** a starting app over ST-Link:
   ```bash
   cd target && cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=./arm-none-eabi-gcc.cmake && ninja -C build
   openocd -f board/stm32f4discovery.cfg -c "program build/blink_green/blink_green.elf verify reset exit"
   ```
   The green LED (PD12) blinks.
2. **Wire the UART** (TTL-232RG, 3.3 V): cable **TXD→PB11**, **RXD→PB10**, **GND→GND**
   (USART3, 8E1). Leave VCC and BOOT0 alone. Full table: `docs/end_to_end_test.md`.
3. **Enter the bootloader:** press & hold the **blue button (B1 / PA0)** — the LED
   goes dark (the app armed the RTC magic and reset into the ST ROM bootloader).
4. **Update from the host:**
   ```bash
   cd host
   BootloaderTool list                                    # find the COM port
   BootloaderTool factory-reset -p COM7 -f ../target/build/blink_orange/blink_orange.bin
   ```
   Erase → write → read-back verify → Go; the orange LED (PD13) now blinks.

Or drive the same cycle from the **GUI** (`dotnet run --project BootloaderTool`,
no arguments): it auto-selects the USB-UART cable, shows live connection status,
takes a `.bin` by drag-and-drop or *Browse* (with size + CRC32), and flashes with
inline progress. See **Host GUI** below.

Further reading: **CLI reference** `docs/cli.md` · **diagrams** `docs/diagrams/` ·
**wiring + GUI walkthrough** `docs/end_to_end_test.md` · **design/protocol notes**
`docs/plan.md`.

# Requirements

* ST Microelectronics STM32F407G-DISC1
* C++14 for the device firmware; .NET 8 SDK for the host tool
* **ARM Embedded GCC** (`arm-none-eabi-gcc`) for the firmware — add `bin/` to PATH
* **MinGW-W64 / native GCC** for the firmware unit tests (built for PC, not ARM)
* **CMake** 3.22+ and **Ninja**
* **OpenOCD** for flashing and on-target debugging via ST-Link
* Git (the Google Test framework is fetched into `3rd-party/googletest`)
* Optional: Doxygen + GraphViz for `target/doxygen`; a serial monitor (PuTTY /
  Tera Term) and a logic analyzer for UART debugging

# Overview

This project has three independently built parts, mirroring the layout of the
other projects in this repository (firmware in `target/`, native unit tests in
`tests/`) with two Bootloader-specific additions (`host/`, `docs/`):

* `3rd-party/googletest` — the Google Test/Mock framework (auto-fetched).
* `target/` — **device firmware**, cross-compiled for the Cortex-M4. Built
  separately from the unit tests.
  * `arm-none-eabi-gcc.cmake` — cross-compile toolchain settings.
  * `gcc-options-cxx.txt` — C++ compiler flags (exceptions/RTTI off, etc.).
  * `CMakeLists.txt` — umbrella build: an isolated `stm32_hal` static library
    plus an `add_firmware_app()` helper; builds both blink apps.
  * `blink_green/Src`, `blink_orange/Src` — per-app user code (`main`,
    `Application`, `board/`, `drivers/`).
  * `shared/` — code shared by both apps: the `bootloader/` entry module,
    `Startup/` assembly, the `linker/` script, ISR/syscall plumbing, and
    `stm32f4xx_hal_conf.h`.
  * `doxygen/` — Doxygen configuration for the firmware.
* `tests/` — **unit tests**, built for PC (not ARM), using code from `target/`.
  Contains `Fake/` (HAL stubs), `Mock/` (GMock objects), and the `Test*.cpp`
  files driven by `TestRunner.cpp`.
* `host/` — the **.NET 8 host application** (`BootloaderTool.sln`): the
  `BootloaderTool.Protocol` class library (AN3155, CRC32, serial, factory-reset
  session), the `BootloaderTool` app/CLI, and `BootloaderTool.Tests` (xUnit).
* `docs/` — project plan, coding style, memory map, and end-to-end test notes.

# Usage

### Firmware (cross-compiled, from `target/`)

```bash
cd target
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=./arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Debug
ninja -C build                 # builds blink_green.elf + blink_orange.elf (+ .bin)
ninja -C build blink_green.elf # or build a single app
```

Flash via OpenOCD:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program target/build/blink_green.bin 0x08000000 verify reset exit"
```

### Firmware unit tests (native PC, from the project root)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build
./build/tests/TestRunner
```

### Host application (.NET, from `host/`)

```bash
cd host
dotnet build
dotnet test
dotnet run --project BootloaderTool          # GUI / default
dotnet run --project BootloaderTool -- factory-reset -p COM3 -f factory.bin
```

All VS Code build/debug tasks are defined in `.vscode/tasks.json` and
`.vscode/launch.json`.

### Host GUI

Launching `BootloaderTool` with no arguments opens the Avalonia GUI — a guided,
three-step flow (Device → Firmware → Flash):

* **Auto-detect / auto-probe.** Serial ports are discovered and kept current in
  the background; the USB-UART cable is auto-selected (a real bridge such as
  *"USB Serial Port"* / FTDI / CP210x is ranked above a board's generic virtual
  COM port). The selected port is continuously probed, so *Flash* lights up the
  moment a live bootloader is present — no Refresh or Connect button.
* **Single session.** The probe holds the synced connection open and the flash
  reuses it, so the ST ROM's one-shot `0x7F` sync is sent exactly once per entry.
  Once connected the port is locked for the session; a successful flash clears
  the image so the next one is a deliberate re-selection.
* **Firmware.** Drag-and-drop a `.bin` onto the window (or *Browse*); the card
  shows the file, size, and CRC32. Up to 1 MB (the F407's flash), and the image
  must carry a plausible Cortex-M vector table — wrong-MCU images, data blobs,
  and renamed `.elf`/`.hex` files are rejected before anything is erased.
* **Image identity.** An image may embed a 32-byte `TLFWIMG1` metadata header
  (product tag + version) after its vector table — see
  [docs/image-header.md](docs/image-header.md). The firmware card then shows a
  *Version* row, and the device card shows what the board currently runs (read
  back over the bootloader on connect). A product mismatch blocks the flash
  (red banner); flashing an older version needs an explicit *Flash anyway*
  (orange banner). Unstamped images and erased devices are unaffected — the
  checks only engage when both sides declare. The format is portable to other
  firmwares and bootloaders: anything that can read the first 1 KB can use it.
* **Activity log.** A foldable in-app log records each step. Everything is also
  mirrored to an append-only `bootloader.log` under the per-user data folder
  (`%LOCALAPPDATA%\BootloaderTool` on Windows — *Open log file* in the activity
  sheet opens it), timestamped, with the flashed file name + path and a rule
  between runs, for later inspection.

The GUI shares all protocol/serial code with the CLI via `BootloaderTool.Protocol`.

# GCC options C++

Specific C++ compiler flags for the firmware live in `target/gcc-options-cxx.txt`
(not the CMakeLists). They disable exceptions and RTTI and tighten a few C++
diagnostics, matching the other projects in this repository.

# Notes

* A `Release` firmware build uses `-Os` and Link Time Optimization; binary size
  is printed after each build.
* Unit tests are Debug-only and built for the host PC.
* The host tool requires the serial port to be configured as **8E1** (8 data
  bits, even parity, 1 stop bit) — a hard requirement of the ST bootloader.
