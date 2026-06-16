# End-to-End Test Plan

Validated on hardware **2026-06-16** (STM32F407G-DISC1, ST-Link V2, TTL-232RG-VIP-WE on COM7).

## Hardware wiring

TTL-232RG-VIP-WE (3.3 V logic) → STM32F4-DISCOVERY, USART3:

| Cable wire | Signal | STM32 pin |
|---|---|---|
| Black (pin 1) | GND | any GND |
| Orange (pin 4) | cable TXD | **PB11** (USART3_RX) |
| Yellow (pin 5) | cable RXD | **PB10** (USART3_TX) |
| Red (pin 3) | VCC 3.3 V | **leave unconnected** (board powered via USB) |

Serial config: **8E1** (even parity is mandatory for the ST bootloader). BOOT0 stays **low** — the
software/magic jump performs the system-flash remap itself; BOOT0-high is only the recovery path.

## Buttons (STM32F4-DISCOVERY)

| Button | Label | Net | Role |
|---|---|---|---|
| Black | B2 / RESET | NRST | Hardware reset → normal app boot (magic cleared) |
| Blue | B1 / USER | PA0 (active-high) | Arms factory-reset magic → enters ST bootloader |

## Bootloader sync is one-shot per entry

The ROM bootloader auto-bauds on the **first** `0x7F` after entry (replies ACK) and answers **NACK**
to any further bare `0x7F`. Each bootloader entry gives one clean sync. `info` / `list` tolerate this
(they accept ACK or NACK as "present"); `factory-reset` wants a fresh first-sync ACK, so always
re-enter the bootloader (RESET → blue button) before running it. See plan §10.5.

## Validated procedure (CLI)

1. Flash a starting app via OpenOCD: `program blink_green.elf verify reset exit`. Green LED (PD12) blinks.
2. **Enter bootloader:** press & hold the blue button (~½ s). LED goes dark.
3. `BootloaderTool factory-reset -p COM7 -f blink_orange.bin` → erase → write → read-back verify →
   Go. Orange LED (PD13) blinks.
4. **Re-enter bootloader** (blue button) and `factory-reset … -f blink_green.bin` → green LED blinks.

Verification is by read-back (F4 has no Get-Checksum 0xA1); a matching `Read-back CRC32` line confirms
the image. See plan §10.4.

## Test scenarios

1. Flash green → factory-reset to orange → confirm LED changes — **validated 2026-06-16**
2. Factory-reset orange → green → confirm LED changes — **validated 2026-06-16**
3. Blue-button entry with no host attached → bootloader answers `info` — **validated 2026-06-16**
4. Power-cycle during upload → verify recovery to bootloader mode — pending
5. Upload corrupt image → verify rejection (read-back CRC mismatch) → pending
6. Cable yank mid-write → **shown unrecoverable on F4 2026-06-16** (bootloader stuck mid-frame; see plan §10.6). Tool fails fast with operator guidance; recovery = re-enter bootloader (RESET + blue button) and Retry / re-run. Fail-fast banner **validated on hardware 2026-06-16** (GUI: immediate Failed + actionable Retry message, no poll spin).
