# BootloaderTool — CLI usage

`BootloaderTool` is one executable with two modes: launched **with no
arguments** it opens the Avalonia GUI; launched **with arguments** it runs the
CLI described here.

```
BootloaderTool <command> [options]
# or, from host/ during development:
dotnet run --project BootloaderTool -- <command> [options]
```

Run `BootloaderTool <command> --help` for command-specific options.

> **Serial config is fixed at 8E1** (8 data bits, even parity, 1 stop bit) — a
> hard requirement of the ST bootloader. Only the baud rate is configurable.

## Commands

| Command | Purpose |
|---------|---------|
| `list` | List serial ports and probe each for an ST bootloader. |
| `info` | Query a device in bootloader mode (chip ID, protocol, command set). |
| `factory-reset` | Erase → write → verify a firmware image, then jump to it. |
| `upload` | Write + verify on a device already in bootloader mode (no Go). |
| `verify` | Compare the device's contents over a region against an image. |
| `read` | Read a memory region to a file. |
| `go` | Jump to an application address (Go, 0x21). |
| `stamp` | Post-build: fill in `imageSize` + `headerCrc` in an image's `TLFWIMG1` header (offline, no port). See [image-header.md](image-header.md). |

```
list   [--baud <n>] [--timeout <ms>]
info   -p <port> [--baud <n>] [--timeout <ms>] [--json]
factory-reset -p <port> -f <image.bin> [--addr <hex>] [--no-go] [--force] [--baud <n>] [--timeout <ms>] [--json]
upload -p <port> -f <image.bin> [--addr <hex>] [--force] [--baud <n>] [--timeout <ms>] [--json]
verify -p <port> -f <image.bin> [--addr <hex>] [--baud <n>] [--timeout <ms>] [--json]
read   -p <port> --addr <hex> --len <n> -o <file> [--baud <n>] [--timeout <ms>] [--json]
go     -p <port> [--addr <hex>] [--baud <n>] [--timeout <ms>]
stamp  -f <image.bin> [--json]
```

## Options

| Option | Default | Meaning |
|--------|---------|---------|
| `-p <port>` | — | Serial port (e.g. `COM7`). |
| `-f <image.bin>` | — | Firmware image (raw binary). |
| `-o <file>` | — | Output file (`read`). |
| `--addr <hex>` | `0x08000000` | Target/flash address. |
| `--len <n>` | — | Byte count (`read`). |
| `--no-go` | off | Skip the final Go; leave the device in bootloader mode. |
| `--force` | off | Override the pre-flash gates (image sanity, product mismatch, downgrade) with a warning instead of a hard block. |
| `--baud <n>` | `115200` | UART baud rate (parity stays 8E1). |
| `--timeout <ms>` | `2000` | Per-command serial read/write timeout. |
| `--retries <n>` | `5` | Sync (0x7F) attempts before giving up on a silent device. |
| `--json` | off | Machine-readable JSON Lines progress/results. |

## Exit codes

| Code | Meaning |
|------|---------|
| `0` | Success. |
| `1` | Runtime failure (port open, protocol, I/O, verification). |
| `2` | Usage error (missing or invalid options). |

## Examples

```sh
# Discover which port the cable enumerated as
BootloaderTool list

# Put the device in bootloader mode first (blue button), then:
BootloaderTool info -p COM7
BootloaderTool factory-reset -p COM7 -f blink_orange.bin
BootloaderTool verify       -p COM7 -f blink_orange.bin
BootloaderTool read -p COM7 --addr 0x08000000 --len 7820 -o dump.bin

# Scripting: stream JSON events
BootloaderTool factory-reset -p COM7 -f fw.bin --json
```

## Pre-flash gates (`factory-reset`, `upload`)

Before anything is erased the tool applies the same checks the GUI does:

- **Image sanity** — a plausible Cortex-M vector table (stack pointer in RAM,
  reset handler in flash) and, for stamped images, a header-declared size that
  matches the file. Catches wrong-file mistakes (data blob, another MCU's
  image, renamed `.elf`).
- **Product mismatch** — a stamped image built for a different product than
  the device currently runs (per its `TLFWIMG1` header) is blocked.
- **Downgrade** — a stamped image older than what the device runs is blocked.

Each gate only bites when the needed information exists (an unstamped image or
an erased/read-protected device means no product/version gate). `--force`
turns every gate into a warning — e.g. for deliberately writing a data blob to
a custom `--addr`.

## Notes & gotchas

- **One `0x7F` sync per bootloader entry.** The ST ROM bootloader ACKs only the
  *first* `0x7F` after entry and NACKs later ones; all verbs treat ACK *or*
  NACK as "present" and reuse that single entry for the whole run. A fresh
  entry (RESET + blue button) before a transfer is still the cleanest starting
  point. See `plan.md` §10.5.
- **Verification is read-back on F4.** The STM32F4 bootloader has no
  Get-Checksum (0xA1), so `factory-reset`/`verify` read the region back and
  CRC32-compare it. See `plan.md` §10.4.
- **A mid-transfer cable loss fails fast.** The bootloader cannot resume a
  partial write; the tool reports the loss and you recover by re-entering the
  bootloader and re-running the command. See `plan.md` §10.6.
- **Wiring & GUI walkthrough:** see `end_to_end_test.md`. **Diagrams:**
  `diagrams/`.
