// ----------------------------------------------------------------------------
//  FirmwareImage.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Represents a firmware .bin image to be flashed: raw bytes, target start
//  address, size, and the STM32-compatible CRC32 computed over the contents.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Buffers.Binary;
using BootloaderTool.Protocol.Crc;

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// A firmware image loaded from a raw <c>.bin</c> file. Exposes the bytes to
/// flash, the target start address, the size, and the CRC32 that the STM32
/// hardware CRC peripheral will produce over the same region (used to verify
/// the device after writing).
/// </summary>
public sealed class FirmwareImage
{
    /// <summary>Default application start address (full 1 MB user flash).</summary>
    public const uint DefaultStartAddress = 0x08000000;

    /// <summary>Raw image bytes, in flash order.</summary>
    public byte[] Data { get; }

    /// <summary>Image size in bytes.</summary>
    public int Size => Data.Length;

    /// <summary>Flash address the image is written to and booted from.</summary>
    public uint StartAddress { get; }

    /// <summary>
    /// CRC-32/MPEG-2 over the image, matching the STM32F4 hardware CRC unit
    /// (polynomial 0x04C11DB7, no reflection). Compared against the device's
    /// Get Checksum (0xA1) result after writing.
    /// </summary>
    public uint Crc32 { get; }

    /// <summary>
    /// CRC-32/MPEG-2 over the image padded to a whole number of 32-bit words
    /// with 0xFF (erased flash). This is what the device's Get Checksum (0xA1)
    /// command produces over <see cref="WordCount"/> words, since flash beyond
    /// the image stays erased; equal to <see cref="Crc32"/> for word-aligned
    /// images.
    /// </summary>
    public uint WordAlignedCrc32 { get; }

    /// <summary>
    /// Initial stack pointer — word 0 of the Cortex-M vector table (0 when the
    /// image is too small to contain one). Callers use this for a plausibility
    /// check: a genuine application image points it into device RAM.
    /// </summary>
    public uint InitialStackPointer { get; }

    /// <summary>
    /// Reset handler address — word 1 of the Cortex-M vector table (0 when the
    /// image is too small to contain one). A genuine application image points
    /// it into flash, with the Thumb bit (bit 0) set.
    /// </summary>
    public uint ResetHandler { get; }

    /// <summary>
    /// The "TLFWIMG1" metadata header embedded after the vector table (product
    /// tag + version), or null for images that do not carry one. See
    /// <see cref="ImageHeader"/> for the format.
    /// </summary>
    public ImageHeader? Header { get; }

    /// <summary>
    /// Loads a firmware image from a <c>.bin</c> file on disk.
    /// </summary>
    /// <param name="path">Path to the raw binary image.</param>
    /// <param name="startAddress">Target flash start address.</param>
    public FirmwareImage(string path, uint startAddress = DefaultStartAddress)
        : this(File.ReadAllBytes(path), startAddress)
    {
    }

    /// <summary>
    /// Creates a firmware image from raw bytes (used directly in tests and by
    /// the file-loading constructor).
    /// </summary>
    /// <param name="data">Raw image bytes.</param>
    /// <param name="startAddress">Target flash start address.</param>
    public FirmwareImage(byte[] data, uint startAddress = DefaultStartAddress)
    {
        if (data is null)
            throw new ArgumentNullException(nameof(data));
        if (data.Length == 0)
            throw new ArgumentException("Firmware image is empty.", nameof(data));

        Data = data;
        StartAddress = startAddress;
        Crc32 = new Crc32().Compute(data);

        if (data.Length % 4 == 0)
        {
            WordAlignedCrc32 = Crc32;
        }
        else
        {
            byte[] padded = new byte[WordCount * 4];
            data.CopyTo(padded, 0);
            padded.AsSpan(data.Length).Fill(0xFF); // erased-flash value
            WordAlignedCrc32 = new Crc32().Compute(padded);
        }

        if (data.Length >= 8)
        {
            // Vector-table words are little-endian in flash, regardless of host.
            InitialStackPointer = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(0, 4));
            ResetHandler        = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(4, 4));
        }

        Header = ImageHeader.FindIn(data);
    }

    /// <summary>
    /// Number of 32-bit words spanned by the image, rounded up. This is the
    /// length passed to the device's Get Checksum command, whose CRC unit
    /// operates on whole words.
    /// </summary>
    public uint WordCount => (uint)((Size + 3) / 4);
}
