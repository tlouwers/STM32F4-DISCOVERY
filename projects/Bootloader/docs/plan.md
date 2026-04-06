# UART Bootloader for STM32 — Project Plan

## 1. Overview

Minimal, fast, and robust UART bootloader for the STM32F4 Discovery board. Written in modern C++ (device and protocol library), with a C# desktop application (Avalonia/.NET) for firmware management. Built with CMake/Ninja, tested with GoogleTest, designed for safe forward/backward version updates and reliable recovery on failure.

## 2. Design Principles

1. **Minimal dependencies** — standard library, one serial backend per OS, no heavy frameworks.
2. **Separation of concerns** — device-side bootloader, portable protocol library, CLI tool, and GUI are independent build targets sharing only headers and the protocol library.
3. **Testability first** — every component has a mock-friendly interface; all logic can run on host without hardware.
4. **Robustness over speed** — every flash write is verified, every packet is CRC-checked, every transfer is resumable.
5. **Clarity for presentation** — architecture diagrams, sequence diagrams, and example session transcripts are planned deliverables, not afterthoughts.

## 3. Constraints & Decisions

| Item | Decision |
|---|---|
| Target MCU | STM32F4 Discovery (single target initially) |
| Device language | C++ (modern, freestanding where needed) |
| Protocol library | C++ with thin C ABI wrapper for cross-language reuse |
| CLI tool | C++ (links protocol library directly) |
| GUI application | C# / Avalonia on .NET (Windows 11 primary) — calls protocol library via C ABI or invokes CLI |
| Integrity | CRC32 required on every packet and on full image |
| Authenticity | Optional cryptographic signature (future phase) |
| Update strategy | Single-slot with non-erasable bootloader region; metadata sector marks image state |
| Build system | CMake + Ninja |
| Test framework | GoogleTest + GoogleMock (host-only) |
| CI | Local-only initially |
| Debug/recovery | OpenOCD / ST-Link; logic analyzer on UART lines |

## 4. Tooling Requirements

### 4.1 Embedded (bootloader + example app)

| Tool | Version | Purpose |
|---|---|---|
| `arm-none-eabi-gcc` | 13.x+ | Cross-compiler for Cortex-M4 |
| `arm-none-eabi-newlib` | (bundled) | C/C++ standard library (nano variant for size) |
| `cmake` | 3.22+ | Build system generator |
| `ninja` | 1.11+ | Fast build backend |
| `openocd` | 0.12+ | Flash and debug via ST-Link |
| `stlink-tools` | 1.8+ | Alternative ST-Link CLI (`st-flash`, `st-info`) |
| `gdb-multiarch` | 14+ | (optional) Debugging over OpenOCD GDB server |

### 4.2 Host tests (GoogleTest)

| Tool | Version | Purpose |
|---|---|---|
| `g++` or `clang++` | GCC 13+ / Clang 17+ | Host compiler (C++17) |
| `cmake` | 3.22+ | Build system |
| `ninja` | 1.11+ | Build backend |
| `googletest` | 1.14+ | Unit test framework (fetched via CMake ExternalProject) |

### 4.3 CLI tool (`blcli`)

| Tool | Version | Purpose |
|---|---|---|
| Host C++ compiler | same as above | Builds CLI binary |
| (no additional deps) | — | Serial I/O uses OS API directly (Win32 / termios) |

### 4.4 GUI application (C# / Avalonia)

| Tool | Version | Purpose |
|---|---|---|
| .NET SDK | 8.0+ | Build and run the GUI |
| Avalonia UI | 11.x | Cross-platform XAML UI framework |
| (no additional NuGet packages beyond Avalonia and System.IO.Ports) | | |

### 4.5 Auxiliary / optional

| Tool | Purpose |
|---|---|
| `minicom` / `PuTTY` / `screen` | Manual serial monitor for UART debugging |
| Logic analyzer software (e.g. PulseView / Saleae) | UART protocol inspection |
| `clang-format` | Code formatting (`.clang-format` in repo) |
| `clang-tidy` | Static analysis |
| `cppcheck` | Additional static analysis (optional) |
| `doxygen` | API documentation generation (optional) |

## 5. Development Environment Setup

### 5.1 Ubuntu 24.04

```bash
# Essential build tools
sudo apt update
sudo apt install -y build-essential cmake ninja-build git

# ARM cross-compiler
sudo apt install -y gcc-arm-none-eabi libnewlib-arm-none-eabi

# Flash / debug tools
sudo apt install -y openocd stlink-tools gdb-multiarch

# Serial tools (optional, for manual testing)
sudo apt install -y minicom picocom

# Code quality (optional)
sudo apt install -y clang-format clang-tidy cppcheck

# .NET SDK 8.0 for GUI (optional, only if building GUI on Linux)
sudo apt install -y dotnet-sdk-8.0

# Verify
arm-none-eabi-gcc --version
cmake --version
ninja --version
openocd --version
```

**udev rule for ST-Link** (required for non-root access):

```bash
# /etc/udev/rules.d/99-stlink.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666"
# Reload:
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 5.2 Windows 11

| Tool | Install method |
|---|---|
| Visual Studio 2022 Build Tools | `winget install Microsoft.VisualStudio.2022.BuildTools` — select "Desktop development with C++" workload |
| CMake | Bundled with VS, or `winget install Kitware.CMake` |
| Ninja | Bundled with VS, or `winget install Ninja-build.Ninja` |
| ARM GCC | Download from Arm Developer — add `bin/` to `PATH` |
| OpenOCD | Download from GitHub releases — add to `PATH` |
| ST-Link drivers | Install via STSW-LINK009 |
| .NET SDK 8.0 | `winget install Microsoft.DotNet.SDK.8` |
| Git | `winget install Git.Git` |
| PuTTY / Tera Term | Serial monitor for UART debugging |

### 5.3 Build commands

```bash
# Firmware unit tests (runs on dev machine, from projects/Bootloader/)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build
./build/tests/TestRunner

# Host unit tests (from projects/Bootloader/host/)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build
./build/tests/TestRunnerHost

# Cross-compile bootloader firmware (from projects/Bootloader/bootloader_fw/)
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../shared/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
ninja -C build

# Flash via OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/bootloader_fw.bin 0x08000000 verify reset exit"
```

All VS Code build tasks are defined in `.vscode/tasks.json`.

## 6. STM32F4 Reference Documents

> All reference documents are in the repo-level `datasheets/` folder:
>
> - `stm32f407vg-datasheet.pdf` — pin-out, electrical characteristics, memory map
> - `rm0090-reference-manual.pdf` — registers, peripherals, flash programming
> - `um1472-discovery-board.pdf` — board schematic, jumper config, ST-Link
> - `pm0214-programming-manual.pdf` — Cortex-M4 instruction set, NVIC, system control
> - `es0182-errata.pdf` — known silicon bugs and workarounds
> - `an3155-usart-bootloader.pdf` — ST USART bootloader protocol reference
> - `an4031-dma.pdf` — DMA controller usage
>
> **Known hardware issues to document:**
> - [ ] Flash write parallelism vs. voltage range (must match `FLASH_CR.PSIZE` to supply voltage)
> - [ ] UART overrun on high baud rates without DMA (solution: use DMA RX with IDLE detection)
> - [ ] ST-Link shared UART conflict (PA2/PA3 used by ST-Link VCP on some Discovery revisions)
> - [ ] Watchdog window timing errata (if applicable to chosen IWDG/WWDG config)
> - [ ] Option byte write lock considerations (write-protecting bootloader sectors)

## 7. Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Host (PC / Windows 11)                │
│                                                         │
│  ┌──────────┐    ┌──────────────┐    ┌──────────────┐   │
│  │  GUI App │───▶│ Protocol Lib │◀───│   CLI Tool   │   │
│  │  (C#)    │    │ (C++ / C ABI)│    │   (C++)      │   │
│  └──────────┘    └──────┬───────┘    └──────────────┘   │
│                         │                               │
│                   ┌─────┴──────┐                        │
│                   │ Serial I/O │ (Win32 COM / termios)   │
│                   └─────┬──────┘                        │
└─────────────────────────┼───────────────────────────────┘
                     UART │
┌─────────────────────────┼───────────────────────────────┐
│               STM32F4 Discovery                         │
│                   ┌─────┴──────┐                        │
│                   │  UART HAL  │                        │
│                   └─────┬──────┘                        │
│          ┌──────────────┼──────────────┐                │
│          ▼              ▼              ▼                │
│  ┌─────────────┐ ┌────────────┐ ┌──────────────┐       │
│  │  Transfer   │ │   Image    │ │   Flash      │       │
│  │  Protocol   │ │  Manager   │ │   Writer     │       │
│  └─────────────┘ └────────────┘ └──────────────┘       │
│                         │                               │
│                   ┌─────┴──────┐                        │
│                   │ Recovery   │                        │
│                   │ Controller │                        │
│                   └────────────┘                        │
└─────────────────────────────────────────────────────────┘
```

### 7.1 Device-side components (`bootloader_fw/`)

| Component | Responsibility |
|---|---|
| `TransferProtocol` | Receive framed packets, ACK/NAK, sequence numbers, resume |
| `ImageManager` | Parse image header, validate CRC, manage metadata state transitions |
| `FlashWriter` | Erase, program, read-back-verify; enforce alignment and sector boundaries |
| `RecoveryController` | Check boot pin / backup register, decide boot-vs-update, watchdog handshake |

### 7.2 Host-side components (`host/`)

| Component | Responsibility |
|---|---|
| `protocol-lib` | Portable C++ library: framing, transport, commands, progress callbacks, CRC. Thin C ABI wrapper. |
| `serial-io` | OS-specific serial port (`SerialWin` / `SerialPosix`) behind `ISerial` |
| `blcli` | CLI executable: commands, progress display, reconnect/resume |
| GUI App | C# / Avalonia: visual device management, progress bars, logs, recovery UI |

## 8. UART Protocol

### 8.1 Layers

```
┌──────────────────────┐
│   Command Layer      │  HELLO, START_UPLOAD, DATA, END, VERIFY, COMMIT, ABORT, RECOVER
├──────────────────────┤
│   Transport Layer    │  Sequence numbers, ACK/NAK, retry/timeout, resume offset
├──────────────────────┤
│   Framing Layer      │  0x7E delimiter, length, type, payload, CRC16, byte-stuffing
├──────────────────────┤
│   Physical Layer     │  UART (configurable baud, 8N1)
└──────────────────────┘
```

### 8.2 Packet format

```
┌───────┬────────┬──────┬─────────────────┬───────┬───────┐
│ START │ LENGTH │ TYPE │     PAYLOAD     │ CRC16 │  END  │
│ 0x7E  │ 2 bytes│1 byte│  0..1024 bytes  │2 bytes│ 0x7E  │
└───────┴────────┴──────┴─────────────────┴───────┴───────┘
```

Byte-stuffing: `0x7E` → `0x7D 0x5E`, `0x7D` → `0x7D 0x5D` inside the frame. CRC16 covers LENGTH + TYPE + PAYLOAD.

### 8.3 Command sequence (happy path)

```
Host                          Device
  │── HELLO ────────────────────▶│
  │◀──────────────── HELLO_ACK ──│  (device info, version, state)
  │── START_UPLOAD(size,crc) ───▶│
  │◀────────────────────── ACK ──│  (device erases target sector)
  │── DATA(seq=0, chunk) ──────▶│
  │◀────────────────────── ACK ──│
  │── DATA(seq=1, chunk) ──────▶│
  │◀────────────────────── ACK ──│
  │         ...                  │
  │── END_UPLOAD ──────────────▶│
  │◀────────────────────── ACK ──│
  │── VERIFY ──────────────────▶│  (device checks full-image CRC)
  │◀──────────── VERIFY_OK/FAIL─│
  │── COMMIT ──────────────────▶│  (marks image active, reboots)
  │◀────────────────────── ACK ──│
```

### 8.4 Error / resume flow

```
Host                          Device
  │── DATA(seq=5, chunk) ──────▶│
  │        (timeout, no ACK)     │
  │── DATA(seq=5, chunk) ──────▶│  (retry)
  │◀────────────────────── ACK ──│

  │   (connection lost)          │

  │── HELLO ────────────────────▶│  (reconnect)
  │◀──── HELLO_ACK(resume@seq5)─│  (device reports last good offset)
  │── START_UPLOAD(offset=5*1K)─▶│  (resume)
  │◀────────────────────── ACK ──│
```

## 9. Flash Layout (STM32F4 Discovery — 1 MB flash)

```
┌────────────────────────┬─────────────┐
│ 0x0800_0000            │ Bootloader  │  Sectors 0-3 (64 KB, write-protected)
├────────────────────────┼─────────────┤
│ 0x0801_0000            │ Metadata    │  Sector 4 (64 KB)
├────────────────────────┼─────────────┤
│ 0x0802_0000            │ Application │  Sectors 5-11 (7 × 128 KB = 896 KB)
└────────────────────────┴─────────────┘
```

- Bootloader write-protected via option bytes — never erased during updates.
- Metadata holds: magic, version (major.minor.patch), image size, CRC32, state enum (`EMPTY` / `RECEIVING` / `VALID` / `ACTIVE` / `ROLLBACK`), build timestamp.
- Power loss during flash → state remains `RECEIVING` → bootloader stays in update mode → safe recovery.

Linker scripts: `shared/linker/stm32f4_bootloader.ld` (bootloader), `shared/linker/stm32f4_app.ld` (application).
Memory map details: `docs/memory_map.md`.

## 10. Hardware Optimizations (STM32F4)

### 10.1 DMA for UART

- **UART RX via DMA**: Circular or double-buffer mode with UART IDLE line detection to detect end-of-packet. Frees the CPU during flash erase/write.
- **UART TX via DMA**: Non-blocking ACK/NAK/status responses.
- **Benefit**: Overlaps flash programming time with UART reception.
- **Implementation**: Double-buffer (ping-pong) — while one buffer is being programmed to flash, DMA fills the other from UART.

### 10.2 Hardware CRC32

- **Fixed polynomial**: `0x04C11DB7` (CRC-32/MPEG-2). Initial value `0xFFFFFFFF`, no bit reversal, no final XOR.
- **Usage**: Feed entire application image through HW CRC on VERIFY command.
- **Limitation**: F4 hardware CRC does **not** support CRC16 — packet-level CRC16 remains in software.
- **Portability**: `ICrc` interface — host tests use `SoftwareCrc32`, device uses `Stm32f4HwCrc32`. SW must match HW polynomial exactly.

### 10.3 Optimization data flow

```
                    ┌─────────────────────────────────┐
                    │         DMA Ping-Pong            │
                    │                                  │
  UART RX ──DMA──▶ │  Buffer A ──▶ Flash Program      │
                    │  Buffer B ◀── DMA filling        │
                    │              (simultaneous)      │
                    └─────────────────────────────────┘

  VERIFY command:
    Flash read ──▶ Hardware CRC32 unit ──▶ compare with expected
```

### 10.4 HAL interface implications

`ICrc` implementations:
- `SoftwareCrc32` — portable, used in host tests and protocol library
- `Stm32f4HwCrc32` — uses CRC peripheral, device-only

`IUart` supports:
- `sendDma(buffer, length, callback)` — non-blocking DMA transmit
- `startReceiveDma(buffer, length)` — start DMA circular/double-buffer receive
- `onPacketReceived(callback)` — triggered by IDLE line detection

Software fallback (polling UART) always available for debugging.

## 11. CLI Tool (`blcli`)

### 11.1 Commands

| Command | Description |
|---|---|
| `blcli list` | Scan serial ports, probe for bootloader-mode devices |
| `blcli info -p COM3` | Query device: bootloader version, app version, image state |
| `blcli upload -p COM3 -f fw.bin` | Upload firmware with full progress display |
| `blcli verify -p COM3` | Ask device to verify current image CRC |
| `blcli commit -p COM3` | Mark image active and reboot device |
| `blcli recover -p COM3` | Force device into recovery/bootloader mode |
| `blcli abort -p COM3` | Cancel in-progress transfer |

### 11.2 Progress display

```
$ blcli upload -p COM3 -f firmware.bin

[info]  Connected to STM32F4 Discovery (bootloader v1.2.0, app v3.1.0)
[info]  Image: firmware.bin (87040 bytes, CRC32: 0xA3B2C1D0)

[xfer]  Sending ████████████████████░░░░░░░░░░  68%  59.2 KB / 85.0 KB  │ 11.2 KB/s  │ ETA 2s
[flash] Erasing sector 5 ... done
[flash] Programming ██████████████░░░░░░░░░░░░░░░░  45%

[xfer]  Sending ████████████████████████████████ 100%  85.0 KB / 85.0 KB  │ 11.4 KB/s
[flash] Programming ████████████████████████████████ 100%
[verify] CRC32 OK (0xA3B2C1D0)
[commit] Image marked active. Device rebooting.

Done. Updated to v3.2.0 in 8.4s.
```

### 11.3 Reconnect / retry behavior

1. **Timeout** (no ACK within 2 s): retry same packet up to 3 times.
2. **Serial disconnect**: attempt reconnect every 1 s for up to 30 s; on reconnect send HELLO and resume.
3. **NAK** (CRC mismatch): re-send the specific packet immediately.
4. **Persistent failure**: abort gracefully, report last offset for manual resume.
5. All configurable: `--retries`, `--timeout`, `--reconnect-interval`.

### 11.4 Library reuse

`protocol-lib` exposes a callback-driven API:

```cpp
ProtocolClient client(serial_port);
client.onProgress([](ProgressType type, uint32_t current, uint32_t total) { ... });
client.onLog([](LogLevel level, const char* msg) { ... });
client.upload("firmware.bin");
```

Consumed by `blcli` (direct C++ link) and GUI app (C# P/Invoke on C ABI wrapper, or `blcli --json` subprocess).

## 12. GUI Application (C# / Avalonia)

- Calls `protocol-lib` via C ABI (P/Invoke) or launches `blcli --json` as subprocess.
- Device panel: serial port selector, connection status, bootloader/app version.
- Upload panel: file picker (drag-and-drop), image metadata preview, CRC pre-check.
- Progress panel: dual progress bars (transfer + flash), rate, ETA, scrollable log.
- Recovery panel: force-recovery button, erase, reflash.
- Auto-reconnect on serial disconnect.

## 13. Versioning & Releases

Semantic versioning for bootloader core and firmware images. Image metadata includes version, build-id, and compatibility flags (min-bootloader-version).

---

## 14. Phased Implementation

Each phase is independently buildable, testable, and demo-able.

### Phase 1 — Foundation
**Goal:** Repo scaffold, build system, HAL interfaces, CRC (software + hardware), first passing test.

| Deliverable | Detail |
|---|---|
| Build system | `CMakeLists.txt` (firmware tests), `host/CMakeLists.txt`, toolchain `shared/arm-none-eabi-gcc.cmake` |
| HAL interfaces | `IFlash`, `IUart`, `ICrc`, `IWatchdog`, `ISystem` (C++ pure virtual) |
| CRC32 | Software implementation matching STM32F4 HW CRC polynomial (`0x04C11DB7`) |
| CRC16 | Software implementation for packet framing |
| First test | GoogleTest: CRC32 and CRC16 known-answer tests |

**Exit criteria:** Firmware unit tests pass on host; cross-compile produces a binary.

### Phase 2 — Protocol framing + transport
**Goal:** Framing and transport layers as a portable library, fully tested on host.

| Deliverable | Detail |
|---|---|
| `Framer` | Encode/decode frames, byte-stuffing, CRC16 |
| `Transport` | Sequencing, ACK/NAK, timeouts, retry |
| `MockSerial` | Simulates loss, delays, corruption |
| Tests | Framing edge cases, transport retry/resume |

**Exit criteria:** All unit tests pass on host with MockSerial.

### Phase 3 — Command layer + ImageManager
**Goal:** Command protocol and image metadata state machine, testable end-to-end on host.

| Deliverable | Detail |
|---|---|
| `CommandHandler` | Device-side command dispatch |
| `ProtocolClient` | Host-side high-level API with progress callbacks |
| `ImageManager` | Metadata parsing, CRC validation, state machine (`EMPTY`→`RECEIVING`→`VALID`→`ACTIVE`) |
| Tests | Full upload/verify/commit cycle over MockSerial |

**Exit criteria:** Host integration test: `ProtocolClient` ↔ `CommandHandler` completes a full cycle.

### Phase 4 — FlashWriter + device boot logic
**Goal:** Flash operations, recovery logic, real bootloader binary for STM32F4.

| Deliverable | Detail |
|---|---|
| `FlashWriter` | Erase, program, read-back-verify, alignment enforcement |
| `RecoveryController` | Boot-pin + metadata → boot-app or stay-in-bootloader |
| STM32F4 HAL adapters | `Stm32f4Flash`, `Stm32f4Uart` (polling initially), `Stm32f4Watchdog`, `Stm32f4HwCrc32` |
| Linker scripts | Already in `shared/linker/` — bootloader at `0x08000000`, app at `0x08020000` |
| `bootloader_fw/main.cpp` | Init → recovery check → boot app or enter update mode |

**Exit criteria:** Unit tests pass (mocked HAL); binary on device responds to HELLO over UART.

### Phase 4b — DMA optimization
**Goal:** Enable DMA for UART RX/TX and hardware CRC32 on device.

| Deliverable | Detail |
|---|---|
| DMA UART RX | Circular/double-buffer DMA with IDLE line detection |
| DMA UART TX | Non-blocking DMA transmit for ACK/NAK/status |
| Ping-pong buffers | Receive into buffer A while programming buffer B |
| Hardware CRC32 | Use CRC peripheral for full-image verification |

**Exit criteria:** Upload speed measurably faster with DMA; HW CRC32 matches SW CRC32.

### Phase 5 — CLI tool (`blcli`)
**Goal:** Polished CLI with accurate dual progress and reconnect/resume.

| Deliverable | Detail |
|---|---|
| `blcli` | All 7 commands — lives in `host/src/` |
| Serial backends | `SerialWin` (Win32), `SerialPosix` (termios) |
| Progress display | Dual bars, rate, ETA, verbose/quiet/json |
| Reconnect/resume | Configurable retries, auto-resume from last ACK'd offset |
| C ABI wrapper | `protocol_c.h` / `.cpp` for GUI FFI |

**Exit criteria:** `blcli upload` on real device with disconnect/reconnect mid-transfer works.

### Phase 6 — GUI application
**Goal:** Lightweight Avalonia app for Windows 11 with visual progress and recovery.

| Deliverable | Detail |
|---|---|
| Avalonia project | MVVM, minimal deps — lives in `host/gui/` |
| Panels | Device, Upload, Progress (dual bars), Recovery |
| Protocol integration | P/Invoke on `protocol_c` DLL or `blcli --json` subprocess |
| Auto-reconnect | Handles serial disconnect gracefully |

**Exit criteria:** Connect → upload → watch progress → yank USB → reconnect → resume.

### Phase 7 — Example apps + end-to-end validation
**Goal:** Demonstrate the full update lifecycle using the existing blink apps.

| Deliverable | Detail |
|---|---|
| `blink_green` | LED blink (PD12/LD4) — already scaffolded, linked with `stm32f4_app.ld` |
| `blink_orange` | LED blink (PD13/LD3) — already scaffolded, linked with `stm32f4_app.ld` |
| Two versions | v1.0.0 and v2.0.0 with different blink patterns for each color |
| E2E test | Flash bootloader → upload v1 → verify → upload v2 → verify → power-cycle during upload → recovery |

Full end-to-end test plan: `docs/end_to_end_test.md`.

**Exit criteria:** Full cycle works with both CLI and GUI; documented with terminal transcripts.

### Phase 8 — Documentation & presentation
**Goal:** Diagrams and docs suitable for presentation and onboarding.

| Deliverable | Detail |
|---|---|
| Architecture diagram | ASCII or SVG |
| Sequence diagrams | Happy path, retry, resume, recovery |
| Boot flow / DMA flow | Reset → decision → boot or update; ping-pong buffer illustration |
| Guides | CLI usage, GUI walkthrough, README quick-start |

## 15. Verification Matrix

| Phase | Host test | Hardware test |
|---|---|---|
| 1 | CRC32 + CRC16 unit tests | Cross-compile check |
| 2 | Framer + Transport (mock serial) | — |
| 3 | Command + ImageManager + ProtocolClient (mock serial) | — |
| 4 | FlashWriter + Recovery (mock HAL) | Bootloader responds to HELLO |
| 4b | — | DMA speed test; HW CRC32 vs SW CRC32 match |
| 5 | — | CLI uploads, resumes, shows progress |
| 6 | — | GUI uploads, resumes, shows progress |
| 7 | — | Full lifecycle with two app versions |
| 8 | — | Documentation review |

## 16. Risk & Mitigation

| Risk | Mitigation |
|---|---|
| Flash write fails mid-sector | State stays `RECEIVING`; bootloader remains in update mode; host resumes |
| Power loss during update | Non-erasable bootloader always boots; metadata unchanged until verify completes |
| UART noise / corruption | CRC16 per packet + CRC32 full image; byte-stuffing prevents framing errors |
| Serial disconnect | Auto-reconnect + resume from last ACK'd offset |
| Wrong firmware flashed | Metadata version + compatibility check; VERIFY before COMMIT |
| DMA overrun | Double-buffer with sufficient size; fallback to polling if DMA fails |
| HW CRC polynomial mismatch | Software CRC must match `0x04C11DB7`; unit test validates both produce identical output |
| Bootloader itself needs update | Out of scope; future dual-bootloader approach |

---

Phases 1–3 require **no hardware** — all testing runs on host with mocks.
Phase 4 is the first hardware touchpoint. Phase 4b is an optional optimization pass.
Phases 5–7 build outward. Phase 8 can run in parallel with any later phase.
