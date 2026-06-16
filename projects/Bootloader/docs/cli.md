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

```
list   [--baud <n>] [--timeout <ms>]
info   -p <port> [--baud <n>] [--timeout <ms>] [--json]
factory-reset -p <port> -f <image.bin> [--addr <hex>] [--no-go] [--baud <n>] [--timeout <ms>] [--json]
upload -p <port> -f <image.bin> [--addr <hex>] [--baud <n>] [--timeout <ms>] [--json]
verify -p <port> -f <image.bin> [--addr <hex>] [--baud <n>] [--timeout <ms>] [--json]
read   -p <port> --addr <hex> --len <n> -o <file> [--baud <n>] [--timeout <ms>] [--json]
go     -p <port> [--addr <hex>] [--baud <n>] [--timeout <ms>]
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
| `--baud <n>` | `115200` | UART baud rate (parity stays 8E1). |
| `--timeout <ms>` | `2000` | Per-command serial read/write timeout. |
| `--retries <n>` | `3` | Per-command retry budget. |
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

## Notes & gotchas

- **Run `factory-reset` against a fresh bootloader entry.** The ST ROM
  bootloader ACKs only the *first* `0x7F` after entry; an `info`/`list` on the
  same entry consumes it. Re-enter (RESET + blue button) before a transfer.
  (`info`/`list` themselves tolerate an already-armed device — they accept ACK
  *or* NACK as "present".) See `plan.md` §10.5.
- **Verification is read-back on F4.** The STM32F4 bootloader has no
  Get-Checksum (0xA1), so `factory-reset`/`verify` read the region back and
  CRC32-compare it. See `plan.md` §10.4.
- **A mid-transfer cable loss fails fast.** The bootloader cannot resume a
  partial write; the tool reports the loss and you recover by re-entering the
  bootloader and re-running the command. See `plan.md` §10.6.
- **Wiring & GUI walkthrough:** see `end_to_end_test.md`. **Diagrams:**
  `diagrams/`.
