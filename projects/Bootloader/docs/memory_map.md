# STM32F407VG Memory Map & Linker Values

This file documents the canonical memory sizes and addresses used for the
STM32F407VG device and explains where the numbers come from and what to
adjust if you target a different MCU or change the bootloader layout.

## Sources

All documents are in the repo-level `datasheets/` folder:

- `stm32f407vg-datasheet.pdf` — flash/RAM sizes, base addresses, sector geometry
- `rm0090-reference-manual.pdf` — flash programming, DMA, UART registers

## Canonical values used in this project

| Region | Start | End | Size |
|---|---|---|---|
| Bootloader | `0x08000000` | `0x0800FFFF` | 64 KiB (sectors 0–3, 4 × 16 KB) |
| Metadata | `0x08010000` | `0x0801FFFF` | 64 KiB (sector 4) |
| Application | `0x08020000` | `0x080FFFFF` | ~896 KiB (sectors 5–11) |
| RAM | `0x20000000` | `0x2001FFFF` | 128 KiB (SRAM1) |
| CCM RAM | `0x10000000` | `0x1000FFFF` | 64 KiB |

Total flash: 1 MiB. Total RAM: 192 KiB (128 KiB SRAM1 + 64 KiB CCM).

## Flash sector sizes (STM32F407VG)

| Sectors | Size each | Total |
|---|---|---|
| 0–3 | 16 KiB | 64 KiB — bootloader region |
| 4 | 64 KiB | 64 KiB — metadata region |
| 5–11 | 128 KiB | 896 KiB — application region |

## CCM (Tightly-Coupled) RAM

The STM32F407VG provides a separate CCM region at `0x10000000` (64 KiB). CCM has
usage constraints (different DMA accessibility and initialization requirements).
The project linker scripts declare `CCMRAM` (64 KiB) and `RAM` (128 KiB) separately.

## Linker scripts

- `shared/linker/stm32f4_bootloader.ld` — bootloader at `0x08000000`
- `shared/linker/stm32f4_app.ld` — application at `0x08020000`

## What to adjust and where

- If you change bootloader size or metadata placement, always align start
  and end addresses to sector boundaries and update both linker scripts.
- If you target a different MCU, update the `MEMORY` sections (`ORIGIN` and `LENGTH`).
- If your application requires more stack or heap, increase `_Min_Stack_Size`
  and `_Min_Heap_Size` in the linker scripts.

## Why the split matters

The bootloader and metadata regions are aligned to sector boundaries so that
runtime flash erase/write can operate on whole sectors without disturbing
adjacent regions. The bootloader region can be write-protected via option bytes.

*Recorded: 2026-04-02 (migrated from original bootloader project)*
