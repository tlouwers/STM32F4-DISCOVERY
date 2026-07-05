# Firmware image metadata header ("TLFWIMG1")

A 32-byte header a firmware image embeds just after its vector table, so any
host tool can tell **what** it is about to flash — product and version — and
compare that against **what the device currently runs**, before anything is
erased.

The format is deliberately **portable**: it does not depend on this project's
bootloader, on AN3155, or on the STM32F4. Any bootloader or flasher that can
read the first kilobyte of an image (from a `.bin` file, or back from device
flash) can locate and use it. The firmware side is a single `const` struct —
no post-build tooling required.

## Goals

1. **Identity** — refuse an image built for a different product, even though
   every `.bin` looks alike.
2. **Version visibility** — show "image v1.2.0, device runs v1.3.0" *before*
   flashing; warn on regressions.
3. **Zero infrastructure** — filled in at compile time; works with plain
   CRC-verified binaries; images without the header keep working (the checks
   simply do not engage).

## Layout

32 bytes, little-endian, placed at a **4-byte-aligned** offset within the
first **1 KB** of the image. Hosts locate it by scanning for the magic — the
exact offset is a per-family convention, not a contract (F4: `0x188`, right
after the vector table; larger vector tables on F7/H7 just push it further).

| Offset | Size | Field       | Meaning                                              |
|-------:|-----:|-------------|------------------------------------------------------|
| 0      | 8    | `magic`     | ASCII `TLFWIMG1` (format name + revision)            |
| 8      | 8    | `product`   | ASCII tag, NUL/space padded (e.g. `F4DISCO1`)        |
| 16     | 2    | `major`     | Version, unsigned                                    |
| 18     | 2    | `minor`     | Version, unsigned                                    |
| 20     | 2    | `patch`     | Version, unsigned                                    |
| 22     | 2    | `reserved`  | 0                                                    |
| 24     | 4    | `imageSize` | Total `.bin` size in bytes; **0 = not stamped**      |
| 28     | 4    | `headerCrc` | CRC-32/MPEG-2 over bytes 0–27; **0 = not stamped**   |

`imageSize` and `headerCrc` cannot be computed at compile time; they stay 0
unless the post-build stamp step fills them in:

```
BootloaderTool stamp -f firmware.bin
```

A host accepts a header when the magic matches, the product tag is printable
ASCII, and — when `headerCrc` is non-zero — the self-CRC checks out. In a
stamped image, a non-zero `imageSize` that disagrees with the file size marks
the file as truncated or padded and it is rejected.

Example hexdump of a stamped image (header at the F4 convention offset):

```
00000188  54 4C 46 57 49 4D 47 31  |TLFWIMG1|   magic
00000190  46 34 44 49 53 43 4F 31  |F4DISCO1|   product
00000198  01 00 03 00 02 00 00 00               v1.3.2, reserved
000001A0  00 4A 00 00 96 7E 11 C3               imageSize 18944, headerCrc
```

## Firmware side (C++14, GCC)

One struct, one placement. No build-step changes beyond a linker line.

```cpp
// ImageHeader.hpp — 32-byte firmware identity block (format "TLFWIMG1").
struct ImageHeader
{
    char     magic[8];      ///< "TLFWIMG1"
    char     product[8];    ///< ASCII tag, NUL padded
    uint16_t major;         ///< Version
    uint16_t minor;         ///< Version
    uint16_t patch;         ///< Version
    uint16_t reserved;      ///< 0
    uint32_t imageSize;     ///< 0 = not stamped (filled by a post-build step)
    uint32_t headerCrc;     ///< 0 = not stamped
};
static_assert(sizeof(ImageHeader) == 32, "ImageHeader must be exactly 32 bytes");

// In one .cpp — the linker section pins it right after the vector table.
__attribute__((section(".image_header"), used))
const ImageHeader gImageHeader =
{
    { 'T','L','F','W','I','M','G','1' },
    { 'F','4','D','I','S','C','O','1' },
    1, 3, 2,        // v1.3.2 — bump per release
    0, 0, 0
};
```

Linker script (`.ld`), inside the flash `.text` output section, directly after
the vectors:

```ld
.isr_vector : { KEEP(*(.isr_vector)) } >FLASH
.image_header : { KEEP(*(.image_header)) } >FLASH   /* lands at 0x188 on F4 */
```

`KEEP` is essential: nothing references the struct, so without it the linker
garbage-collects the section. The exact resulting offset does not matter to
hosts (they scan), but keeping it directly after the vectors is the
convention and keeps it inside the 1 KB scan window.

## Host side — what BootloaderTool does with it

Implemented in `BootloaderTool.Protocol/Protocol/ImageHeader.cs` (shared by
GUI and CLI):

* **Loading a `.bin`** — `FirmwareImage` scans for the header. The firmware
  card shows a **Version** row: `v1.3.2 · F4DISCO1` for a stamped image, or
  `not stamped` for a plain binary. The card tints **green** as before once a
  valid image is loaded — the header only adds information and gates, it never
  blocks a plain image.
* **Connecting a device** — after identifying the bootloader, the probe reads
  the first 1 KB of application flash back and scans it the same way. The
  device card then reports e.g. `runs F4DISCO1 v1.3.0` next to the chip ID.
  An erased or legacy device simply shows no version.
* **Product mismatch** (both sides stamped, different tags) — a **red** banner
  in the flash card names both products and flashing is blocked.
* **Downgrade** (same product, image version older than the device) — an
  **orange** banner: *"The device runs v1.3.0; this image is v1.2.0 (older)."*
  The main flash button stays greyed; flashing requires the deliberate
  **Flash anyway** click in the banner. Rollback stays possible — it just can
  no longer happen by accident.
* **Either side unstamped** — no gate. The header is additive.

## Porting notes

* The header is **bootloader-agnostic**: locating it needs only "read the
  first 1 KB", which every flashing transport provides (AN3155 Read Memory,
  SWD, a custom bootloader's read command, or plain file I/O).
* Per MCU family, only the *convention offset* changes (wherever the vector
  table ends); hosts need no change because they scan.
* The product tag is the compatibility contract between an image and a board.
  Choose one 8-char tag per product and never reuse it.
* `magic` doubles as the format version — an incompatible layout change means
  a new magic (`TLFWIMG2`), so old and new hosts never misparse each other.
* Devices with readout protection (RDP) refuse the flash read-back; hosts
  must degrade to "version unknown", not fail the connection.
