// ----------------------------------------------------------------------------
//  ImageHeader.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Firmware image metadata header ("TLFWIMG1", 32 bytes): product tag, semantic
//  version, optional image size and self-CRC. Embedded by the firmware right
//  after its vector table; located by the host by scanning the first 1 KB for
//  the magic, so the exact offset may differ per MCU family. Lets the host
//  refuse an image built for another product and warn about version downgrades
//  before anything is erased. See docs/image-header.md for the full format.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

using System.Buffers.Binary;
using System.Text;
using BootloaderTool.Protocol.Crc;

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// The 32-byte "TLFWIMG1" metadata header a firmware image may embed just after
/// its vector table. Identifies the product (8-char ASCII tag) and semantic
/// version so a host tool can display what it is about to flash, refuse an
/// image built for a different product, and warn about downgrades. The format
/// is deliberately bootloader-agnostic: any tool that can read the first
/// kilobyte of an image (from file or from device flash) can use it.
///
/// Layout (little-endian, 4-byte aligned):
/// <code>
/// offset  size  field
///   0      8    magic       "TLFWIMG1" (format name + revision)
///   8      8    product     ASCII tag, NUL/space padded (e.g. "F4DISCO1")
///  16      2    major       version, unsigned
///  18      2    minor       version, unsigned
///  20      2    patch       version, unsigned
///  22      2    reserved    0
///  24      4    imageSize   total .bin size in bytes; 0 = not stamped
///  28      4    headerCrc   CRC-32/MPEG-2 over bytes 0..27; 0 = not stamped
/// </code>
/// <c>imageSize</c> and <c>headerCrc</c> are optional (0 when the header is
/// filled in at compile time with no post-build stamping step).
/// </summary>
public sealed class ImageHeader
{
    /// <summary>The 8-byte magic marking a metadata header: "TLFWIMG1".</summary>
    public static readonly byte[] Magic =
        { (byte)'T', (byte)'L', (byte)'F', (byte)'W', (byte)'I', (byte)'M', (byte)'G', (byte)'1' };

    /// <summary>Size of the header structure in bytes.</summary>
    public const int Size = 32;

    /// <summary>
    /// How far into an image <see cref="FindIn"/> scans for the magic. One
    /// kilobyte covers the vector table of every Cortex-M family in use here
    /// (F4: 0x188, larger on F7/H7) with room to spare, while keeping a device
    /// read-back of this region fast at bootloader baud rates.
    /// </summary>
    public const int ScanLimit = 1024;

    /// <summary>Product tag: up to 8 ASCII characters, padding trimmed.</summary>
    public string Product { get; }

    /// <summary>Major version.</summary>
    public ushort Major { get; }

    /// <summary>Minor version.</summary>
    public ushort Minor { get; }

    /// <summary>Patch version.</summary>
    public ushort Patch { get; }

    /// <summary>Declared total image size in bytes; 0 when not stamped.</summary>
    public uint ImageSize { get; }

    /// <summary>Byte offset within the scanned data at which the header was found.</summary>
    public int Offset { get; }

    /// <summary>Version formatted for display, e.g. "v1.3.2".</summary>
    public string VersionText => $"v{Major}.{Minor}.{Patch}";

    private ImageHeader(string product, ushort major, ushort minor, ushort patch,
                        uint imageSize, int offset)
    {
        Product   = product;
        Major     = major;
        Minor     = minor;
        Patch     = patch;
        ImageSize = imageSize;
        Offset    = offset;
    }

    /// <summary>
    /// Compares this header's version to another's (product is not considered).
    /// </summary>
    /// <param name="other">Header to compare against.</param>
    /// <returns>Negative when this version is older, 0 when equal, positive when newer.</returns>
    public int CompareVersionTo(ImageHeader other)
    {
        if (other is null)
            throw new ArgumentNullException(nameof(other));

        int result = Major.CompareTo(other.Major);
        if (result != 0)
            return result;
        result = Minor.CompareTo(other.Minor);
        return result != 0 ? result : Patch.CompareTo(other.Patch);
    }

    /// <summary>
    /// Scans the start of an image (or of device flash read back over a
    /// bootloader) for a valid metadata header. Word-aligned offsets within the
    /// first <see cref="ScanLimit"/> bytes are checked for the magic; a match
    /// must then also carry a printable product tag and — when stamped — a
    /// correct self-CRC, so stray magic bytes in code do not parse as a header.
    /// </summary>
    /// <param name="data">Image bytes; only the first <see cref="ScanLimit"/> bytes are examined.</param>
    /// <returns>The parsed header, or null when none is present.</returns>
    public static ImageHeader? FindIn(ReadOnlySpan<byte> data)
    {
        int end = Math.Min(data.Length, ScanLimit) - Size;
        for (int offset = 0; offset <= end; offset += 4)
        {
            if (!data.Slice(offset, Magic.Length).SequenceEqual(Magic))
                continue;

            ImageHeader? header = TryParse(data.Slice(offset, Size), offset);
            if (header is not null)
                return header;
        }
        return null;
    }

    /// <summary>Parses and validates one 32-byte candidate; null when invalid.</summary>
    private static ImageHeader? TryParse(ReadOnlySpan<byte> raw, int offset)
    {
        // Product: 8 ASCII bytes, NUL/space padded, at least one visible char.
        ReadOnlySpan<byte> tag = raw.Slice(8, 8);
        foreach (byte b in tag)
        {
            bool printable = b >= 0x20 && b <= 0x7E;
            if (!printable && b != 0)
                return null;
        }
        string product = Encoding.ASCII.GetString(tag).TrimEnd('\0', ' ');
        if (product.Length == 0)
            return null;

        // Self-CRC over bytes 0..27; 0 means "not stamped" and is accepted.
        uint headerCrc = BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(28, 4));
        if (headerCrc != 0 && new Crc32().Compute(raw.Slice(0, 28)) != headerCrc)
            return null;

        return new ImageHeader(
            product,
            BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(16, 2)),
            BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(18, 2)),
            BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(20, 2)),
            BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(24, 4)),
            offset);
    }
}
