# STM32F407VG Memory Map & Linker Values

This file documents the canonical memory sizes and addresses used for the
STM32F407VG device under the current architecture (**Option A** — the ST system
memory bootloader handles flashing; there is no custom bootloader in user
flash). See `plan.md` §1a and §9 for the rationale.

> **Superseded layout:** Earlier revisions of this project reserved sectors 0–3
> for a custom bootloader and sector 4 for metadata, with the application at
> `0x08020000`. That split was abandoned when the project adopted the ST system
> memory bootloader. The application now owns the **entire 1 MB** of flash and
> starts at `0x08000000`.

## Sources

All documents are in the repo-level `datasheets/` folder:

- `stm32f407vg-datasheet.pdf` — flash/RAM sizes, base addresses, sector geometry
- `rm0090-reference-manual.pdf` — flash programming, DMA, UART registers

## Canonical values used in this project

| Region | Start | End | Size |
|---|---|---|---|
| System memory (ST ROM) | `0x1FFF0000` | `0x1FFF7FFF` | 29 KiB (factory bootloader, read-only) |
| Application | `0x08000000` | `0x080FFFFF` | 1 MiB (sectors 0–11, full flash) |
| RAM | `0x20000000` | `0x2001FFFF` | 128 KiB (SRAM1) |
| CCM RAM | `0x10000000` | `0x1000FFFF` | 64 KiB |

Total flash: 1 MiB. Total RAM: 192 KiB (128 KiB SRAM1 + 64 KiB CCM).

## Flash sector sizes (STM32F407VG)

| Sectors | Size each | Total |
|---|---|---|
| 0–3 | 16 KiB | 64 KiB |
| 4 | 64 KiB | 64 KiB |
| 5–11 | 128 KiB | 896 KiB |

The application linker script places the vector table at the start of sector 0
(`0x08000000`). A factory reset erases the sectors the factory image occupies
(via AN3155 Extended Erase) and rewrites the image; the host derives the sector
list from the image start address and size (see `Stm32F4FlashLayout` in the host
tool).

## CCM (Tightly-Coupled) RAM

The STM32F407VG provides a separate CCM region at `0x10000000` (64 KiB). CCM has
usage constraints (different DMA accessibility and initialization requirements).
The linker script declares `CCMRAM` (64 KiB) and `RAM` (128 KiB) separately.

## Linker script

- `target/shared/linker/stm32f4_app.ld` — application, `FLASH_APP` at
  `0x08000000`, length 1024 KiB.

## What to adjust and where

- If you target a different MCU, update the `MEMORY` section (`ORIGIN` and
  `LENGTH`) in `stm32f4_app.ld`.
- If the application requires more stack or heap, increase `_Min_Stack_Size`
  and `_Min_Heap_Size` in the linker script.

*Recorded: 2026-04-02. Updated 2026-06-14 for the Option A (system memory
bootloader) architecture.*
