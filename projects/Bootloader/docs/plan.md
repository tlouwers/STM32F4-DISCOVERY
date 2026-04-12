# UART Bootloader for STM32 — Project Plan

## 1. Overview

Minimal, fast, and robust UART bootloader for the STM32F4 Discovery board. The host-side tool is a standalone .NET 8 desktop application using **Avalonia UI** — a single C# codebase covering protocol library, serial I/O, CLI mode, and GUI. Device-side application firmware (with bootloader-entry module) is C++ built with CMake/Ninja. Designed for safe forward/backward version updates and reliable recovery on failure.

---

## 1a. ST System Memory Bootloader — Key Discovery

> **Discovery (2026-04-07):** The STM32F407 has a factory-programmed bootloader burned into system memory (`0x1FFF0000`, 29 KB). This changes the architectural options for the project. This section documents what it provides and what the impact is.

### What the ST system memory bootloader provides (STM32F407, bootloader v9.x)

Activated by **BOOT0 = 1, BOOT1 = 0** (Pattern 1) on reset. Two bootloader versions exist for the F407:
- **V3.x** — USART1/3, CAN2, USB DFU
- **V9.x** — All of V3.x plus I2C1/2/3, SPI1/2 (newer silicon and WLCSP90 package only; V9.1 is in all packages)

**USART configuration (V9.x, as used by AN3155):**

| Parameter | Value |
|---|---|
| Interfaces | USART1 (PA9/PA10), USART3 (PB10/PB11 or PC10/PC11) |
| Frame format | **8-bit data, even parity, 1 stop bit** |
| Baud rate | Auto-detected via 0x7F sync byte |
| System clock | 60 MHz via PLL (HSI source for USART) |

Other interfaces available: CAN2 (125 kbps), I2C1/2/3 (up to 400 kHz, target addr 0x74), SPI1/2 (slave, 8 MHz max), USB DFU.

**USART protocol summary (AN3155):**

| Element | Value |
|---|---|
| Sync / baud detect | Host sends `0x7F` → device replies ACK (`0x79`) |
| ACK | `0x79` |
| NACK | `0x1F` |
| Command framing | `CMD` byte + `~CMD` byte (NOT checksum) |
| Address framing | 4-byte big-endian + XOR checksum byte |
| Data framing | `N-1` byte + N bytes + XOR checksum |
| Max data per Write | 256 bytes |

**Supported commands:**

| Command | Code | Description |
|---|---|---|
| Get | 0x00 | Returns protocol version + list of supported commands |
| Get Version | 0x01 | Returns protocol version |
| Get ID | 0x02 | Returns chip ID (e.g. 0x0413 = STM32F40x/41x) |
| Read Memory | 0x11 | Read up to 256 bytes from any address |
| Go | 0x21 | Jump to address (boot application) |
| Write Memory | 0x31 | Write up to 256 bytes to flash or RAM |
| Erase Memory | 0x43 | Erase 1..N sectors (legacy) |
| Extended Erase | 0x44 | Erase 1..N sectors with 2-byte sector addressing |
| Write Protect | 0x63 | Enable write protection on sectors |
| Write Unprotect | 0x73 | Disable write protection on all sectors |
| Readout Protect | 0x82 | Enable read protection |
| Readout Unprotect | 0x92 | Disable read protection |
| Get Checksum | 0xA1 | Compute CRC on a memory region (multiples of 4 bytes) |

**Key differences from the custom protocol designed in Sections 7–8:**

| Aspect | Current plan (custom) | ST AN3155 protocol |
|---|---|---|
| Framing | 0x7E delimiters, byte-stuffing | No framing layer — command + NOT(CMD) checksum |
| Packet format | START \| LEN \| TYPE \| PAYLOAD \| CRC16 \| END | CMD \| ~CMD → ACK → payload → XOR checksum |
| UART config | 8N1 (no parity) | **8E1 (even parity)** — hardware change required |
| Max data/write | 1024 bytes | 256 bytes per Write Memory command |
| Full-image CRC | CRC32, after transfer | Get Checksum command (CRC on any memory region) |
| Resume / retry | Custom sequence numbers + offset resume | Re-run Write Memory from last failed address |
| Command set | HELLO/START/DATA/END/VERIFY/COMMIT/ABORT | Get/ReadMem/WriteMem/Erase/Go/GetChecksum |
| Boot mode entry | RecoveryController (software decision) | BOOT0 pin = 1 on reset (hardware pin) |

### The official Open Bootloader middleware (stm32-mw-openbl)

ST also publishes [stm32-mw-openbl](https://github.com/STMicroelectronics/stm32-mw-openbl), an IAP (In-Application Programming) C library:
- Runs from **user flash** (not system memory) — flash it like any application
- **Fully protocol-compatible with the system memory bootloader** (same AN3155, AN4221, AN4286, etc.)
- Works with STM32CubeProgrammer out of the box
- Customizable: choose which interfaces, memory regions, and commands to expose
- Provides `Core/`, `Interfaces/Patterns/` (reference implementations), `Interfaces/Templates/` (starting points)
- Relies on STM32Cube HAL/LL for hardware init

### Architectural options

Three approaches are now possible:

**Option A — Use the system memory bootloader directly (simplest)**
- No device-side firmware work at all
- Host sends BOOT0 assertion signal (e.g. via DTR/RTS line or dedicated GPIO) before reset
- Host tool implements AN3155 protocol
- *Impact on plan*: Phases 1–4 (device firmware) are replaced by hardware setup only; protocol library switches to AN3155; UART config changes to 8E1

**Option B — Build a custom bootloader, compatible with AN3155 (recommended)**
- Device-side bootloader implements the AN3155 command set (or a subset)
- Host tool implements AN3155 — same as Option A on the host side
- Compatible with STM32CubeProgrammer as a drop-in test tool during development
- Can use stm32-mw-openbl as reference implementation or starting point
- Falls back to system memory bootloader if custom bootloader is corrupt (BOOT0 hardware path)
- *Impact on plan*: Protocol changes (custom → AN3155); framing layer and custom commands replaced; 8E1 UART instead of 8N1

**Option C — Keep the fully custom protocol (original plan)**
- Continue as originally designed
- Educationally rich, demonstrates protocol design from scratch
- Not compatible with STM32CubeProgrammer or the factory bootloader
- Dual-mode option: implement AN3155 alongside custom protocol
- *Impact on plan*: No changes needed; ST documents are reference material only

### Decision: **Option A — System memory bootloader, host-side focus**

> **Chosen direction (2026-04-07):** Use the ST system memory bootloader directly. No custom device-side bootloader firmware. All effort goes into a polished, robust, branded host-side tool.
>
> **Rationale:**
> - Flash space is free — no bootloader occupies user flash
> - The ST bootloader is battle-hardened; correctness of the erase/write/verify cycle is not our problem to solve
> - The interesting engineering is on the host side: reliable factory reset triggering, recovery from partial state, clear user feedback
> - The host tool can work with any STM32 board, not just this one — broader applicability
> - "Branded" means the GUI/CLI experience, not the firmware

### Reference documents (now in `datasheets/`)

| Document | File | Content |
|---|---|---|
| AN2606 | `an2606-system-memory-boot-mode.pdf` | System memory bootloader activation, peripheral config per device |
| AN3155 | `an3155-usart-bootloader-protocol.pdf` | USART protocol command set (primary reference) |
| AN3155 legacy | `cd00264342-usart-bootloader-protocol-legacy.pdf` | Earlier revision |
| AN4221 | `an4221-i2c-bootloader-protocol.pdf` | I2C protocol |
| AN4286 | `an4286-spi-bootloader-protocol.pdf` | SPI protocol |
| USB DFU | `cd00264379-usb-dfu-protocol.pdf` | USB DFU protocol |
| Open BL repo | https://github.com/STMicroelectronics/stm32-mw-openbl | ST open bootloader middleware |

---

## 2. Design Principles

1. **Use proven infrastructure** — the ST system memory bootloader handles flash erase, write, and verify; we build on top of it, not around it.
2. **Separation of concerns** — device-side entry module, host protocol library, CLI, and GUI are independent; they share only headers and the protocol library.
3. **Testability first** — every host component has a mock-friendly interface; the full factory reset flow can be simulated on host without hardware.
4. **Robustness over speed** — every stage of the factory reset has a defined success/failure state; partial operations are detectable and recoverable.
5. **UX is the deliverable** — the host tool is what the user sees; clarity of progress, clean error messages, and graceful recovery are first-class concerns.

## 3. Constraints & Decisions

| Item | Decision |
|---|---|
| Target MCU | STM32F4 Discovery (single target initially) |
| Device-side bootloader | ST system memory bootloader (factory ROM, no custom firmware) |
| Bootloader entry | Software jump from application (magic backup register pattern) or BOOT0 pin; see Section 7.1 |
| Protocol | AN3155 USART (8-bit, even parity, 1 stop bit, auto-baud via 0x7F sync) |
| Host application | Single .NET 8 + Avalonia UI solution (C#) — protocol library, serial, CLI mode, and GUI in one codebase |
| CLI mode | Same application binary with `--cli` flag or verb commands (no separate executable) |
| GUI mode | Default launch — Avalonia desktop window |
| Integrity | Per-packet XOR checksum (AN3155 built-in); full image verified via AN3155 Get Checksum (0xA1) command |
| Authenticity | Optional cryptographic signature on firmware binary (future phase) |
| Update strategy | Erase application sectors → write factory image → verify CRC → Go command |
| Build system | CMake + Ninja |
| Test framework | xUnit (.NET host app), GoogleTest + GoogleMock (device-side C++ firmware) |
| CI | Local-only initially |
| Debug/recovery | OpenOCD / ST-Link; logic analyzer on UART lines |

## 4. Tooling Requirements

### 4.1 Embedded (application firmware with bootloader-entry module)

| Tool | Version | Purpose |
|---|---|---|
| `arm-none-eabi-gcc` | 13.x+ | Cross-compiler for Cortex-M4 |
| `arm-none-eabi-newlib` | (bundled) | C/C++ standard library (nano variant for size) |
| `cmake` | 3.22+ | Build system generator |
| `ninja` | 1.11+ | Fast build backend |
| `openocd` | 0.12+ | Flash and debug via ST-Link (initial flashing) |
| `stlink-tools` | 1.8+ | Alternative ST-Link CLI (`st-flash`, `st-info`) |
| `gdb-multiarch` | 14+ | (optional) Debugging over OpenOCD GDB server |

### 4.2 Host application (.NET 8 + Avalonia)

| Tool | Version | Purpose |
|---|---|---|
| .NET SDK | 8.0+ | Build, test, and publish the host application |
| Avalonia UI | 11.x | Cross-platform XAML-based desktop UI framework |
| xUnit | 2.x | Unit test framework for protocol library and serial |
| System.IO.Ports | 8.x | Cross-platform serial port access (NuGet) |
| CommunityToolkit.Mvvm | 8.x | MVVM source generators and helpers (NuGet) |

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

# ARM cross-compiler (for device firmware only)
sudo apt install -y gcc-arm-none-eabi libnewlib-arm-none-eabi

# Flash / debug tools
sudo apt install -y openocd stlink-tools gdb-multiarch

# Serial tools (optional, for manual testing)
sudo apt install -y minicom picocom

# Code quality (optional)
sudo apt install -y clang-format clang-tidy cppcheck

# .NET SDK 8.0 for host application
sudo apt install -y dotnet-sdk-8.0

# Verify
arm-none-eabi-gcc --version
cmake --version
ninja --version
openocd --version
dotnet --version
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
| .NET SDK 8.0 | `winget install Microsoft.DotNet.SDK.8` — builds host application |
| Git | `winget install Git.Git` |
| PuTTY / Tera Term | Serial monitor for UART debugging |

### 5.3 Build commands

```bash
# Firmware unit tests (runs on dev machine, from projects/Bootloader/)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build
./build/tests/TestRunner

# Host application — build (from projects/Bootloader/host/)
dotnet build

# Host application — run tests
dotnet test

# Host application — run GUI (default)
dotnet run --project src/BootloaderTool

# Host application — run CLI mode
dotnet run --project src/BootloaderTool -- factory-reset -p COM3 -f factory.bin

# Host application — publish self-contained (Windows x64)
dotnet publish src/BootloaderTool -c Release -r win-x64 --self-contained

# Flash application firmware via OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/app_firmware.bin 0x08000000 verify reset exit"
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
> - `an3155-usart-bootloader.pdf` — ST USART bootloader protocol (older version already present)
> - `an3155-usart-bootloader-protocol.pdf` — **AN3155 Rev 21 (Feb 2026) — primary USART protocol reference**
> - `an2606-system-memory-boot-mode.pdf` — **System memory bootloader activation + STM32F407 peripheral config**
> - `an4221-i2c-bootloader-protocol.pdf` — I2C bootloader protocol
> - `an4286-spi-bootloader-protocol.pdf` — SPI bootloader protocol
> - `cd00264379-usb-dfu-protocol.pdf` — USB DFU bootloader protocol
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
│  ┌────────────────────────────────────────────────────┐  │
│  │      BootloaderTool (.NET 8 + Avalonia)             │  │
│  │                                                     │  │
│  │  ┌──────────┐    ┌──────────────┐                   │  │
│  │  │  GUI     │───▶│ Protocol Lib │◀── CLI mode       │  │
│  │  │ (Avalonia│    │ (C# classes) │    (verb commands) │  │
│  │  │  MVVM)   │    └──────┬───────┘                   │  │
│  │  └──────────┘           │                           │  │
│  │                   ┌─────┴──────┐                    │  │
│  │                   │ Serial I/O │ (System.IO.Ports)  │  │
│  │                   └─────┬──────┘                    │  │
│  └─────────────────────────┼──────────────────────────┘  │
└─────────────────────────────┼───────────────────────────┘
                         UART │ (8E1, auto-baud)
┌─────────────────────────────┼───────────────────────────┐
│               STM32F4 Discovery                         │
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │         System Memory Bootloader (ST ROM)         │   │
│  │         0x1FFF0000, 29 KB, V9.1                  │   │
│  │   USART1/3, I2C1/2/3, SPI1/2, CAN2, USB DFU     │   │
│  └──────────────────────────────────────────────────┘   │
│                         ▲                               │
│                    BOOT0=1  or  software jump            │
│                         │                               │
│  ┌──────────────────────┴───────────────────────────┐   │
│  │           Application Firmware                    │   │
│  │   ┌─────────────────────────────────────────┐    │   │
│  │   │  BootloaderEntry module (~50 lines C++)  │    │   │
│  │   │  - Listens for factory reset command     │    │   │
│  │   │  - Writes magic to RTC backup register   │    │   │
│  │   │  - Performs system reset                 │    │   │
│  │   │  - Startup: detects magic → jumps to ROM │    │   │
│  │   └─────────────────────────────────────────┘    │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 7.1 Device-side: BootloaderEntry module

The only device-side code is a small module added to the application firmware. No separate bootloader binary.

**Startup hook** (runs before any peripheral init, checks for magic value):
```cpp
// In startup / Reset_Handler, before SystemInit():
constexpr uint32_t kFactoryResetMagic = 0xDEADBEEF;
if (RTC->BKP0R == kFactoryResetMagic) {
    RTC->BKP0R = 0;
    JumpToSystemMemory();   // never returns
}
```

**JumpToSystemMemory():**
```cpp
void JumpToSystemMemory() {
    __disable_irq();
    // Deinit all peripherals cleanly
    HAL_RCC_DeInit();
    HAL_DeInit();
    // Remap system memory to 0x00000000
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    // Set SP and jump to system memory reset vector
    const uint32_t* sysMem = reinterpret_cast<const uint32_t*>(0x1FFF0000);
    __set_MSP(sysMem[0]);
    reinterpret_cast<void(*)()>(sysMem[1])();
}
```

**Factory reset trigger** (application command handler):
```cpp
void OnFactoryResetCommand() {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    RTC->BKP0R = kFactoryResetMagic;
    HAL_NVIC_SystemReset();
}
```

**BOOT0 pin alternative:** For hardware-triggered entry (e.g. holding a button during power-on), pull BOOT0 (pin 94 on LQFP100, connected to B2 on Discovery) high before reset. The ST system memory bootloader starts automatically — no application code needed.

### 7.2 Host-side components (`host/`) — .NET 8 + Avalonia

Single .NET solution (`BootloaderTool.sln`) with the following projects:

| Project | Responsibility |
|---|---|
| `BootloaderTool` | Avalonia desktop app (GUI mode) and CLI entry point (verb commands). Single published executable. |
| `BootloaderTool.Protocol` | Class library: AN3155 implementation, CRC32, `ISerial` interface, `FirmwareImage`, `FactoryResetSession` |
| `BootloaderTool.Tests` | xUnit tests for protocol library: CRC32, AN3155 commands, factory reset session, mock serial |

## 8. Protocol — AN3155 USART

### 8.1 Physical layer

UART: **8 data bits, even parity, 1 stop bit (8E1)**. This is a hard requirement from the ST bootloader. Any host serial port must be configured with even parity or all bytes will be received with parity errors.

### 8.2 Baud rate detection

Host sends `0x7F` at the desired baud rate. The ST bootloader measures the pulse width with SysTick and configures its UART accordingly. It then ACKs with `0x79`. Supported range: ~1200 to 115200 baud. Recommended: **115200**.

### 8.3 Command framing

Every command is sent as two bytes: `CMD` followed by `~CMD` (bitwise NOT). The bootloader verifies both bytes before responding.

```
Host sends:  [ CMD ] [ ~CMD ]
Device:      [ ACK ]          ← if valid command and RDP not active
             then payload exchange follows
```

Address framing (4 bytes + XOR checksum):
```
[ A3 ] [ A2 ] [ A1 ] [ A0 ] [ A3^A2^A1^A0 ]
```

Data write framing:
```
[ N-1 ] [ D0 ] [ D1 ] ... [ DN-1 ] [ (N-1)^D0^D1^...^DN-1 ]
```
Maximum N = 256 bytes per Write Memory command.

### 8.4 Command set (used by this project)

| Command | Code | Usage |
|---|---|---|
| Get | 0x00 | Enumerate supported commands, confirm bootloader is alive |
| Get ID | 0x02 | Read chip ID (0x0413 = STM32F40x/41x) — device identification |
| Get Version | 0x01 | Read protocol version |
| Extended Erase | 0x44 | Erase application sectors before flashing |
| Write Memory | 0x31 | Write up to 256 bytes at a time; repeated for full image |
| Get Checksum | 0xA1 | Verify full image CRC after write (device computes CRC over flash) |
| Go | 0x21 | Jump to application start address after successful flash |
| Read Memory | 0x11 | Optional: read back regions for debug or verification |

### 8.5 Factory reset flow (happy path)

```
Host                              Device (ST system memory BL)
  │                                   │
  │  [trigger: send factory-reset]    │
  │──────────────────────────────────▶│  (application writes magic, resets)
  │                                   │  ST bootloader starts on reset
  │  [poll: send 0x7F × N]           │
  │──────────────────────────────────▶│
  │◀─────────────────────── ACK ──────│  (baud rate detected, ready)
  │                                   │
  │── Get (0x00, 0xFF) ──────────────▶│
  │◀──── ACK + command list + ACK ───│  (confirm Extended Erase, Write, etc.)
  │                                   │
  │── Get ID (0x02, 0xFD) ──────────▶│
  │◀──── ACK + 0x04 + 0x13 + ACK ───│  (chip ID = 0x0413, STM32F40x)
  │                                   │
  │── Extended Erase (0x44, 0xBB) ──▶│
  │    [sector list for app region]  │
  │◀─────────────────────── ACK ──────│  (erase complete, may take seconds)
  │                                   │
  │── Write Memory × N chunks ──────▶│  (256 bytes each, address increments)
  │◀─────────────────────── ACK ──────│  (per chunk)
  │         ...                       │
  │                                   │
  │── Get Checksum (0xA1, 0x5E) ────▶│
  │    [start addr, length, poly]    │
  │◀──── ACK + CRC32 + ACK ──────────│  (compare with expected CRC)
  │                                   │
  │── Go (0x21, 0xDE) ──────────────▶│
  │    [app start address]           │
  │◀─────────────────────── ACK ──────│  (device jumps, bootloader exits)
  │                                   │  Application running
```

### 8.6 Error and retry behaviour

- **No ACK within timeout** (configurable, default 2 s): retransmit command, up to 3 retries
- **NACK received** (0x1F): log error code, abort operation, report to user
- **Connection lost during erase**: reconnect loop (poll 0x7F every 1 s for up to 60 s) — device is still in bootloader if BOOT0 held or magic in RTC register
- **Checksum mismatch**: erase and retry the full write; do not Go
- **Persistent failure**: abort, report last known state, leave device in bootloader mode for manual intervention

## 9. Flash Layout (STM32F4 Discovery — 1 MB flash)

```
┌────────────────────────┬─────────────────────────────────────────────┐
│ 0x1FFF_0000            │ System Memory (ST ROM, 29 KB)               │  ← not in user flash
│                        │ ST bootloader V9.1 — read-only, permanent   │
├────────────────────────┼─────────────────────────────────────────────┤
│ 0x0800_0000            │ Application                                 │  Sectors 0-11 (all 1 MB)
│  (sector 0)            │ Start address for linker script             │
│    ...                 │   Sectors 0-1: vector table + startup code  │
│  (sector 11)           │   Sector 2+: app code and data              │
└────────────────────────┴─────────────────────────────────────────────┘
```

- No bootloader in user flash. The full 1 MB is available to the application.
- No metadata sector needed — the factory image is the complete known-good state.
- Factory reset erases selected sectors (via Extended Erase 0x44) and rewrites the factory binary.
- The "factory image" is a `.bin` file bundled with or selected in the host tool.

**Sector map (STM32F407, 1 MB variant):**

| Sector | Address | Size |
|---|---|---|
| 0 | 0x0800_0000 | 16 KB |
| 1 | 0x0800_4000 | 16 KB |
| 2 | 0x0800_8000 | 16 KB |
| 3 | 0x0800_C000 | 16 KB |
| 4 | 0x0801_0000 | 64 KB |
| 5 | 0x0802_0000 | 128 KB |
| 6–11 | 0x0804_0000+ | 128 KB each |

Linker script: `shared/linker/stm32f4_app.ld` (application starts at `0x08000000`).
Memory map details: `docs/memory_map.md`.

## 10. Hardware Considerations

### 10.1 UART configuration: 8E1 (mandatory)

The ST system memory bootloader configures USART with **even parity**. The host serial port must match exactly:
- 8 data bits, even parity, 1 stop bit
- Any other configuration causes silent parity errors and the bootloader will not respond

USART1 pins on Discovery: **PA9** (TX), **PA10** (RX).
USART3 alternative: PB10/PB11 or PC10/PC11.

### 10.2 Bootloader entry: two mechanisms

**Software trigger (preferred for factory reset UX):**
The running application receives a factory reset command, writes a magic value to RTC Backup Register 0, then resets. On the next startup, the application checks the register before any peripheral init and jumps to system memory if the magic is present. No hardware changes required; works over the existing UART connection.

**BOOT0 pin (fallback / hardware button):**
BOOT0 is wired to user button B2 on the Discovery board (through JP3 — check board revision). Holding BOOT0 high during reset enters system memory bootloader unconditionally. Useful as a recovery path if application firmware is corrupt.

### 10.3 Erase timing

Extended Erase of the full 1 MB (sectors 0–11) takes **several seconds** on STM32F4 (flash erase is slow). The ST bootloader sends ACK only after erase is complete. The host must use a long timeout for the erase command (30–60 s). Erasing only the sectors that the factory image occupies reduces this time.

### 10.4 Host-side CRC

The AN3155 Get Checksum command (0xA1) uses the STM32 hardware CRC peripheral. The polynomial is `0x04C11DB7` (CRC-32/MPEG-2), initial value `0xFFFFFFFF`, input and output not reflected. The host protocol library must compute the same CRC on the factory image binary before sending, to compare with the device's result.

`ICrc` interface on host:
- `SoftwareCrc32` — used in host tests and pre-flight verification of the firmware binary
- Matches STM32F4 hardware CRC peripheral output exactly — validated by a known-answer test

## 11. CLI Mode

The same `BootloaderTool` executable supports CLI mode when invoked with verb commands. No separate binary.

### 11.1 Commands

| Command | Description |
|---|---|
| `BootloaderTool list` | Scan serial ports, probe for ST bootloader-mode devices (send 0x7F, wait for ACK) |
| `BootloaderTool info -p COM3` | Query device in bootloader mode: chip ID, protocol version, supported commands |
| `BootloaderTool factory-reset -p COM3 -f factory.bin` | Full factory reset: erase → write → verify → go |
| `BootloaderTool upload -p COM3 -f fw.bin` | Write firmware only (device already in bootloader mode) |
| `BootloaderTool verify -p COM3 -f fw.bin` | Run Get Checksum and compare against file CRC |
| `BootloaderTool read -p COM3 --addr 0x08000000 --len 1024 -o dump.bin` | Read memory region to file |
| `BootloaderTool go -p COM3 --addr 0x08000000` | Jump to application |

### 11.2 Progress display

```
$ blcli factory-reset -p COM3 -f factory_v1.0.bin

[info]  Triggering factory reset on device...
[info]  Waiting for ST bootloader...  connected (115200 baud, 8E1)
[info]  Chip ID: 0x0413 (STM32F405/407/415/417)  Protocol: v3.1
[info]  Image: factory_v1.0.bin  (87040 bytes, CRC32: 0xA3B2C1D0)

[erase]  Erasing sectors 0-4 ... ████████████████████████████████ done  (4.2s)
[write]  Writing  ████████████████████░░░░░░░░░░  68%  59.2 KB / 85.0 KB  │ 10.8 KB/s  │ ETA 2s
[write]  Writing  ████████████████████████████████ 100%  85.0 KB  │ 11.1 KB/s
[verify] CRC32 OK (0xA3B2C1D0)
[boot]   Jumping to 0x08000000 ... done

Factory reset complete in 12.1s.
```

### 11.3 Reconnect / retry behaviour

1. **No ACK within timeout** (default 2 s per command): retry up to 3 times, then abort.
2. **Serial disconnect**: reconnect loop — send 0x7F every 1 s for up to 60 s; on success, restart from erase.
3. **NACK**: log error with command context; abort and report clearly.
4. **Checksum mismatch**: re-run erase + write (do not attempt Go).
5. **Erase timeout**: use 60 s timeout for Extended Erase; show "Erasing..." heartbeat to user.
6. All configurable: `--retries`, `--timeout`, `--reconnect-timeout`.

### 11.4 Library reuse

`BootloaderTool.Protocol` exposes a callback-driven API:

```csharp
var session = new FactoryResetSession(serialPort);
session.Progress += (stage, current, total) => { ... };
session.Log += (level, msg) => { ... };
await session.RunAsync(new FirmwareImage("factory.bin"));
```

Consumed directly by both CLI mode and GUI (same process, same assembly — no FFI or subprocess).

## 12. GUI (Avalonia, default launch mode)

The GUI is the default launch mode of `BootloaderTool`. It calls `BootloaderTool.Protocol` directly (same process, no FFI).

- **Device panel**: serial port selector (auto-refreshing), connect button, chip ID and protocol version display once connected.
- **Factory Reset panel**: firmware file picker (drag-and-drop), CRC and size preview, single "Factory Reset" button.
- **Progress panel**: progress bar with stage label (Connecting → Erasing → Writing → Verifying → Done), KB/s rate, ETA, scrollable log.
- **Error panel**: plain-language error message, Retry button, log export, manual Go / Read buttons for diagnostics.
- Auto-reconnect: if serial disconnects mid-operation, shows reconnect countdown and retries automatically.
- **MVVM architecture**: views in AXAML, view models use `CommunityToolkit.Mvvm`, protocol interactions on background threads with progress marshalled to UI thread.

## 13. Versioning & Releases

Semantic versioning for bootloader core and firmware images. Image metadata includes version, build-id, and compatibility flags (min-bootloader-version).

---

## 14. Phased Implementation

Each phase is independently buildable, testable, and demo-able. Phases 1–3 require no hardware. Phase 4 is the first hardware touchpoint.

### Phase 1 — Foundation (complete — C++ prototype)
**Goal:** Repo scaffold, build system, CRC32 implementation, first passing test.

| Deliverable | Detail | Status |
|---|---|---|
| Build system | `host/CMakeLists.txt`, toolchain `shared/arm-none-eabi-gcc.cmake` (for app firmware only) | Done |
| `ICrc` interface | Pure virtual; `SoftwareCrc32` implementation (C++) | Done |
| CRC32 | Software, polynomial `0x04C11DB7` (matches STM32F4 hardware CRC unit) | Done |
| `ISerial` / `SerialWin` / `SerialPosix` | C++ serial port abstraction | Done |
| First test | GoogleTest: CRC32 known-answer test | Done |

**Exit criteria:** CRC32 unit test passes; host build produces a test binary. ✓

> **Note:** Phase 1 was implemented in C++ as a prototype. The C++ host code has been removed — all host functionality is now in the .NET solution. The C++ GoogleTest infrastructure at the project root (`tests/`) remains for device-side firmware unit tests.

### Phase 2 — .NET solution + AN3155 protocol library (complete)
**Goal:** Create the .NET 8 + Avalonia solution, port CRC32 and serial, implement the full AN3155 protocol — all tested with xUnit.

| Deliverable | Detail |
|---|---|
| Solution scaffold | `host/BootloaderTool.sln` with projects: `BootloaderTool` (app), `BootloaderTool.Protocol` (class lib), `BootloaderTool.Tests` (xUnit) |
| `ICrc` / `Crc32` | C# port of STM32-compatible CRC32 (`0x04C11DB7`, no reflection) |
| `ISerial` | C# interface: `Open`, `Close`, `Write`, `Read`, `SetTimeout` |
| `SerialPortAdapter` | `System.IO.Ports` implementation of `ISerial` (8E1 config) |
| `MockSerial` | xUnit-friendly mock that simulates ACK/NACK, timeouts, connection loss |
| `An3155Client` | Implements all commands: Sync, Get, GetID, ExtendedErase, WriteMemory, GetChecksum, Go, ReadMemory |
| Retry + timeout logic | Configurable retries (default 3), per-command timeout, reconnect loop |
| Progress events | `Progress` event (`stage`, `current`, `total`), `Log` event (`level`, `msg`) |
| xUnit tests | CRC32 known-answer, sync/baud detect, each command happy path, NACK handling, timeout+retry, connection loss during erase |

**Exit criteria:** All AN3155 commands pass xUnit tests with MockSerial; CRC32 matches STM32 hardware output; `dotnet test` green. ✓

### Phase 3 — Factory reset orchestrator
**Goal:** High-level `FactoryResetSession` that sequences the full operation, tested end-to-end on host.

| Deliverable | Detail |
|---|---|
| `FirmwareImage` | Load `.bin` from file, compute CRC32, expose metadata (size, CRC, start address) |
| `FactoryResetSession` | Orchestrates: sync → get → getID → erase → write (chunked) → checksum verify → go |
| State machine | States: `Idle` → `Connecting` → `Erasing` → `Writing` → `Verifying` → `Booting` → `Done` / `Failed` |
| Mock end-to-end test | `FactoryResetSession` runs to completion over `MockSerial`; simulates mid-write disconnect and retry |

**Exit criteria:** Full factory reset sequence completes and verifies successfully on host with MockSerial.

### Phase 4 — Application firmware + BootloaderEntry module
**Goal:** Minimal application firmware with the BootloaderEntry module; first real hardware test. Unit tests use GoogleTest.

| Deliverable | Detail |
|---|---|
| `BootloaderEntry` | Startup magic check, `JumpToSystemMemory()`, `TriggerFactoryReset()` command handler |
| Startup hook | Checks RTC BKP0R before `SystemInit()`; jumps to `0x1FFF0000` if magic present |
| App firmware | Simple blink app that exposes the factory reset command over UART (at app baud rate) |
| Linker script | `shared/linker/stm32f4_app.ld` — app starts at `0x08000000` |
| Unit tests | GoogleTest tests for BootloaderEntry logic (magic register check, jump guard conditions) |

**Exit criteria:** Sending factory reset command from PC causes device to jump to ST bootloader; host polls 0x7F and gets ACK.

### Phase 5 — GUI + CLI mode
**Goal:** Polished Avalonia GUI (default) and CLI mode in the same executable.

| Deliverable | Detail |
|---|---|
| Avalonia GUI | MVVM: serial port selector, firmware file picker, device info panel, progress bar with stage labels, ETA, scrollable log, error recovery panel |
| CLI mode | Verb commands: `factory-reset`, `upload`, `verify`, `info`, `read`, `go`, `list` — console progress bar |
| Serial backend | `SerialPortAdapter` using `System.IO.Ports` — 8E1, configurable baud |
| Reconnect loop | Auto-reconnect on serial loss; configurable timeout and interval |
| `--json` output | Machine-readable progress events (CLI mode) for scripting |

**Exit criteria:** GUI: click "Factory Reset" → watch all stages → cable yank mid-write → reconnect → resume → completion. CLI: `BootloaderTool factory-reset -p COM3 -f factory.bin` completes on real hardware.

### Phase 6 — End-to-end validation
**Goal:** Demonstrate the full workflow using two versions of the blink app.

| Deliverable | Detail |
|---|---|
| `blink_v1` | Green LED blink (PD12/LD4), version 1.0.0 |
| `blink_v2` | Orange LED blink (PD13/LD3), version 2.0.0 |
| E2E test | Flash v1 → factory-reset to v2 → verify LED changes → factory-reset to v1 → verify → power-cycle during erase → recovery |

Full end-to-end test plan: `docs/end_to_end_test.md`.

**Exit criteria:** Both CLI mode and GUI complete the full cycle; documented with terminal transcripts.

### Phase 7 — Documentation & presentation
**Goal:** Diagrams and docs suitable for presentation and onboarding.

| Deliverable | Detail |
|---|---|
| Architecture diagram | ASCII or SVG: host stack → UART → ST bootloader ROM → flash |
| Sequence diagrams | Happy path, retry, mid-write reconnect, erase timeout handling |
| Boot flow diagram | Reset → BKP0R check → JumpToSystemMemory vs. normal boot |
| Guides | CLI usage, GUI walkthrough, README quick-start, hardware wiring notes |

## 15. Verification Matrix

| Phase | Host test | Hardware test |
|---|---|---|
| 1 | CRC32 known-answer test (GoogleTest, C++ prototype) | Cross-compile check |
| 2 | CRC32 + AN3155 all commands (xUnit, MockSerial) — happy path + failure modes | — |
| 3 | FactoryResetSession end-to-end over MockSerial; mid-write disconnect + retry | — |
| 4 | BootloaderEntry unit tests (GoogleTest) | ST bootloader responds to 0x7F sync after software jump from app |
| 5 | — | GUI + CLI `factory-reset` completes; disconnect mid-write recovers |
| 6 | — | Full lifecycle with `blink_v1` and `blink_v2`; CLI and GUI |
| 7 | — | Documentation review |

## 16. Risk & Mitigation

| Risk | Mitigation |
|---|---|
| ST bootloader not responding | Check BOOT0 pin and parity (8E1); verify baud rate; use logic analyser on PA9/PA10 |
| Erase timeout (30+ seconds for full flash) | Use long erase timeout (60 s); poll in a loop with progress heartbeat to user |
| Power loss during erase | Device stays in ST bootloader mode (BOOT0 held or magic in BKP0R not cleared); host reconnects and retries from erase |
| Power loss during write | ST bootloader does not resume mid-write; re-run erase + full write from address 0 |
| CRC mismatch after write | Re-run full erase + write; do not issue Go; report error to user |
| Serial disconnect mid-transfer | Reconnect loop (poll 0x7F every 1 s for up to 60 s); resume from erase if reconnect succeeds |
| Wrong firmware binary loaded | Pre-flight: verify binary size and CRC shown to user before write; Get ID confirms chip matches |
| 8E1 parity not configured on host | Error at first byte; detect by timeout + log clear message "Check serial parity (must be even)" |
| BKP0R not preserved across reset | Ensure `HAL_PWR_EnableBkUpAccess()` is called before writing; verify on startup with a short read |
| App firmware too large for slot | Pre-flight: compare binary size against available flash and refuse if too large |

---

Phases 1–3 require **no hardware** — all testing runs on host with MockSerial.
Phase 4 is the first hardware touchpoint.
Phases 5–6 build outward. Phase 7 runs in parallel with any later phase.
