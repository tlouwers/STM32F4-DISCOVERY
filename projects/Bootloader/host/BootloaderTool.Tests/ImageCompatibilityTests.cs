// ----------------------------------------------------------------------------
//  ImageCompatibilityTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for the shared pre-flash gates: vector-table plausibility,
//  header-declared size, product mismatch, and version downgrade.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

using System.Buffers.Binary;
using System.Text;
using BootloaderTool.Protocol.Protocol;
using Xunit;

namespace BootloaderTool.Tests;

public class ImageCompatibilityTests
{
    /// <summary>The F4 convention offset: right after the 0x188-byte vector table.</summary>
    private const int ConventionOffset = 0x188;

    /// <summary>Builds an image with a plausible vector table (SP in SRAM, reset in flash).</summary>
    private static byte[] PlausibleImage(
        int size = 2048, uint sp = 0x20005000, uint reset = 0x08000101)
    {
        byte[] image = new byte[size];
        BinaryPrimitives.WriteUInt32LittleEndian(image.AsSpan(0), sp);
        BinaryPrimitives.WriteUInt32LittleEndian(image.AsSpan(4), reset);
        return image;
    }

    /// <summary>Builds a raw 32-byte unstamped header (no size, no self-CRC).</summary>
    private static byte[] RawHeader(string product, ushort major, ushort minor, ushort patch,
                                    uint imageSize = 0)
    {
        byte[] h = new byte[ImageHeader.Size];
        ImageHeader.Magic.CopyTo(h, 0);
        Encoding.ASCII.GetBytes(product.PadRight(8, '\0')).CopyTo(h, 8);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(16), major);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(18), minor);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(20), patch);
        BinaryPrimitives.WriteUInt32LittleEndian(h.AsSpan(24), imageSize);
        return h;
    }

    /// <summary>Parses a header instance from raw bytes (for the header-pair checks).</summary>
    private static ImageHeader Header(string product, ushort major, ushort minor, ushort patch)
    {
        byte[] image = new byte[64];
        RawHeader(product, major, minor, patch).CopyTo(image, 0);
        return ImageHeader.FindIn(image)!;
    }

    // ── CheckVectorTable ─────────────────────────────────────────────────────

    [Fact]
    public void CheckVectorTable_PlausibleImage_ReturnsNull()
    {
        var image = new FirmwareImage(PlausibleImage());

        Assert.Null(ImageCompatibility.CheckVectorTable(image));
    }

    [Fact]
    public void CheckVectorTable_StackPointerInCcm_ReturnsNull()
    {
        var image = new FirmwareImage(PlausibleImage(sp: 0x10008000));

        Assert.Null(ImageCompatibility.CheckVectorTable(image));
    }

    [Fact]
    public void CheckVectorTable_TooSmall_ReturnsReason()
    {
        var image = new FirmwareImage(new byte[4]);

        Assert.Contains("too small", ImageCompatibility.CheckVectorTable(image));
    }

    [Fact]
    public void CheckVectorTable_StackPointerNotInRam_ReturnsReason()
    {
        var image = new FirmwareImage(PlausibleImage(sp: 0x00000000));

        Assert.Contains("stack pointer", ImageCompatibility.CheckVectorTable(image));
    }

    [Fact]
    public void CheckVectorTable_ResetHandlerMissingThumbBit_ReturnsReason()
    {
        var image = new FirmwareImage(PlausibleImage(reset: 0x08000100));

        Assert.Contains("reset handler", ImageCompatibility.CheckVectorTable(image));
    }

    [Fact]
    public void CheckVectorTable_ResetHandlerOutsideFlash_ReturnsReason()
    {
        var image = new FirmwareImage(PlausibleImage(reset: 0x20000001));

        Assert.Contains("reset handler", ImageCompatibility.CheckVectorTable(image));
    }

    // ── CheckDeclaredSize ────────────────────────────────────────────────────

    [Fact]
    public void CheckDeclaredSize_DeclaredMatchesFile_ReturnsNull()
    {
        byte[] data = PlausibleImage(size: 2048);
        RawHeader("F4DISCO1", 1, 0, 0, imageSize: 2048).CopyTo(data, ConventionOffset);

        Assert.Null(ImageCompatibility.CheckDeclaredSize(new FirmwareImage(data)));
    }

    [Fact]
    public void CheckDeclaredSize_DeclaredDiffersFromFile_ReturnsReason()
    {
        byte[] data = PlausibleImage(size: 2048);
        RawHeader("F4DISCO1", 1, 0, 0, imageSize: 999).CopyTo(data, ConventionOffset);

        Assert.Contains("damaged", ImageCompatibility.CheckDeclaredSize(new FirmwareImage(data)));
    }

    [Fact]
    public void CheckDeclaredSize_UnstampedSize_ReturnsNull()
    {
        byte[] data = PlausibleImage(size: 2048);
        RawHeader("F4DISCO1", 1, 0, 0, imageSize: 0).CopyTo(data, ConventionOffset);

        Assert.Null(ImageCompatibility.CheckDeclaredSize(new FirmwareImage(data)));
    }

    // ── CheckProduct ─────────────────────────────────────────────────────────

    [Fact]
    public void CheckProduct_SameProduct_ReturnsNull()
    {
        Assert.Null(ImageCompatibility.CheckProduct(
            Header("F4DISCO1", 1, 0, 0), Header("F4DISCO1", 2, 0, 0)));
    }

    [Fact]
    public void CheckProduct_DifferentProduct_ReturnsReason()
    {
        string? reason = ImageCompatibility.CheckProduct(
            Header("OTHERPRD", 1, 0, 0), Header("F4DISCO1", 1, 0, 0));

        Assert.Contains("OTHERPRD", reason);
        Assert.Contains("F4DISCO1", reason);
    }

    [Fact]
    public void CheckProduct_EitherHeaderMissing_ReturnsNull()
    {
        Assert.Null(ImageCompatibility.CheckProduct(null, Header("F4DISCO1", 1, 0, 0)));
        Assert.Null(ImageCompatibility.CheckProduct(Header("F4DISCO1", 1, 0, 0), null));
        Assert.Null(ImageCompatibility.CheckProduct(null, null));
    }

    // ── CheckDowngrade ───────────────────────────────────────────────────────

    [Fact]
    public void CheckDowngrade_OlderImage_ReturnsReason()
    {
        string? reason = ImageCompatibility.CheckDowngrade(
            Header("F4DISCO1", 1, 2, 3), Header("F4DISCO1", 1, 3, 0));

        Assert.Contains("v1.3.0", reason);
        Assert.Contains("v1.2.3", reason);
    }

    [Fact]
    public void CheckDowngrade_SameVersion_ReturnsNull()
    {
        Assert.Null(ImageCompatibility.CheckDowngrade(
            Header("F4DISCO1", 1, 2, 3), Header("F4DISCO1", 1, 2, 3)));
    }

    [Fact]
    public void CheckDowngrade_NewerImage_ReturnsNull()
    {
        Assert.Null(ImageCompatibility.CheckDowngrade(
            Header("F4DISCO1", 2, 0, 0), Header("F4DISCO1", 1, 9, 9)));
    }

    [Fact]
    public void CheckDowngrade_EitherHeaderMissing_ReturnsNull()
    {
        Assert.Null(ImageCompatibility.CheckDowngrade(null, Header("F4DISCO1", 1, 0, 0)));
        Assert.Null(ImageCompatibility.CheckDowngrade(Header("F4DISCO1", 1, 0, 0), null));
    }
}
