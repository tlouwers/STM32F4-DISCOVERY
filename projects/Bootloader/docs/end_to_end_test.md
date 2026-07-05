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

1. Flash green → factory-reset to orange → confirm LED changes — **validated 2026-06-16**, re-validated 2026-07-05 (fixed tool)
2. Factory-reset orange → green → confirm LED changes — **validated 2026-06-16**, re-validated 2026-07-05 (fixed tool)
3. Blue-button entry with no host attached → bootloader answers `info` — **validated 2026-06-16**
4. Power-cycle during upload → verify recovery to bootloader mode — **validated 2026-07-05.** Unplugging the board's own USB cable (power) mid-write, then reconnecting, surfaced a clean GUI error — `Unexpected response 0x3F to command 0x31` (the device rebooted mid-frame; a stray byte, not a clean ACK/NACK) — rather than a hang. Recovery to bootloader mode via RESET + blue button **did not work** (the app flash was left partially corrupted by the interrupted write, so it cannot service the button-poll code that triggers bootloader entry); recovery via **OpenOCD/SWD reflash** succeeded immediately. Same root cause as scenario 6 (no mid-write resume on F4), now confirmed for power loss specifically, not just a cable yank.
5. Upload corrupt image → verify rejection (read-back CRC mismatch) → **closed 2026-07-05 without a live repro.** Forcing a genuine *post-write* CRC mismatch deterministically (without an actual timeout) would need silent mid-transfer byte corruption, which isn't reproducible safely on this 3-wire link. Closed instead on: (a) the new pre-flight `ImageCompatibility` gate (L55), which rejects an implausible/corrupt image *before* any erase/write — demonstrated live on hardware (`upload` blocked a 16-byte junk blob with a clear message, proceeded only with `--force`); (b) existing unit coverage of the write-verify-retry-exhaustion path itself (`FactoryResetSessionTests`, scripted mismatched CRC → `ChecksumMismatchException` after `MaxWriteAttempts`).
6. Cable yank mid-write → **shown unrecoverable on F4 2026-06-16** (bootloader stuck mid-frame; see plan §10.6). Tool fails fast with operator guidance; recovery = re-enter bootloader (RESET + blue button) and Retry / re-run. Fail-fast banner **validated on hardware 2026-06-16** (GUI: immediate Failed + actionable Retry message, no poll spin).

## L53/L54/L55 fix verification (2026-07-05)

Hardware session re-running the full plan against the fixed host tool (127→154 tests green), STM32F407G-DISC1 on COM7, same wiring as above. Two throwaway test images (`blink_orange`/`blink_green`, stamped `TLFWIMG1` F4DISCO1 v2.0.0 / v1.0.0) were built for the downgrade-gate test — see `target/blink_{orange,green}/Src/ImageHeader.cpp` and the `.image_header` linker section added to `target/shared/linker/stm32f4_app.ld` (opt-in, no-op for apps that don't define the symbol).

- **L53 partial-read discard, timeout-restore, empty-sectors guard, word-aligned CRC** — exercised indirectly through every factory-reset/upload run below; no regressions. `--timeout 8000` flowed through a full erase→write→verify cycle without incident (the differential proof that the *correct* prior value is restored, not a hardcoded one, lives in the unit tests — real command round-trips here are too fast to force an observable difference).
- **L54 fix 1 (stale downgrade confirmation)** — armed a downgrade (device v2.0.0 → image v1.0.0), forced a failure via a brief RX wire disturbance mid-write (`Timeout waiting for ACK to command 0x31`), confirmed **Retry stayed disabled** (bound to the same `CanFlashFirmware` gate) until **Flash anyway** was clicked again. Matches the fix exactly: `_downgradeConfirmed` is cleared unconditionally in the flash `finally`. Recovery from the resulting stuck bootloader was via OpenOCD (expected — same as scenario 6/no mid-write resume). Second attempt (wire left alone) completed cleanly.
- **L54 fix 2 (close-mid-flash port leak)** — closed the GUI window mid-write; process exited cleanly (exit code 0); a separate CLI process opened COM7 immediately afterward and got a clean `info` response — port released and device left un-stuck (cancellation lands between 256-byte chunks, not mid-frame).
- **L55 CLI pre-flight gates** — `upload` on a 16-byte junk file was blocked pre-port-open with a clear message; `--force` turned it into a warning and the (nonsensical) write proceeded and verified. Product/downgrade gating reuses the same probe-then-check flow as the GUI (confirmed via the downgrade-gate test above, since `factory-reset`'s device-header read is exercised there too).

### Findings from this session (not yet fixed)

- **GUI layout shift on Card 3** — the amber downgrade banner (and the error banner stacking below it) grows Card 3 tall enough to force the window's vertical scrollbar, rather than the card holding a constant height (violates the project's no-ui-shift-on-state-change rule). Pre-existing, not introduced by L53–L55.
- **No in-GUI reconnect after a stuck bootloader session** — once a failure keeps `_connection` "usable" (by design, so Retry is instant) but the *hardware* has since been power-cycled or RESET, there is no way to force a fresh probe short of restarting the app or a port-list change (unplug/replug the adapter). `RestartProbe()` only fires from `OnSelectedPortChanged` or a flash's own success/lost-connection path. Worth a small explicit "Reconnect" affordance.
- **GUI taskbar/window icon is still the default** — cosmetic, flagged separately by Terry.
