// ----------------------------------------------------------------------------
//  ImageHeaderTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for ImageHeader: magic scan (alignment, scan limit), field
//  parsing, product-tag validation, optional self-CRC, and version comparison.
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
using BootloaderTool.Protocol.Protocol;
using Xunit;

namespace BootloaderTool.Tests;

public class ImageHeaderTests
{
    /// <summary>The F4 convention offset: right after the 0x188-byte vector table.</summary>
    private const int ConventionOffset = 0x188;

    /// <summary>Builds a raw 32-byte header. crc null = unstamped (0).</summary>
    private static byte[] MakeHeader(
        string product = "F4DISCO1",
        ushort major = 1, ushort minor = 2, ushort patch = 3,
        uint imageSize = 0, uint? crc = null, bool selfCrc = false)
    {
        byte[] h = new byte[ImageHeader.Size];
        ImageHeader.Magic.CopyTo(h, 0);
        Encoding.ASCII.GetBytes(product.PadRight(8, '\0')).CopyTo(h, 8);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(16), major);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(18), minor);
        BinaryPrimitives.WriteUInt16LittleEndian(h.AsSpan(20), patch);
        BinaryPrimitives.WriteUInt32LittleEndian(h.AsSpan(24), imageSize);

        uint headerCrc = selfCrc ? new Crc32().Compute(h.AsSpan(0, 28)) : (crc ?? 0);
        BinaryPrimitives.WriteUInt32LittleEndian(h.AsSpan(28), headerCrc);
        return h;
    }

    /// <summary>Returns a zero-filled image with the header copied in at the offset.</summary>
    private static byte[] Embed(byte[] header, int offset, int totalLength)
    {
        byte[] image = new byte[totalLength];
        header.CopyTo(image, offset);
        return image;
    }

    [Fact]
    public void FindIn_HeaderAtConventionOffset_ParsesAllFields()
    {
        byte[] image = Embed(MakeHeader("STANDUPC", 4, 5, 6, imageSize: 1234), ConventionOffset, 2048);

        ImageHeader? header = ImageHeader.FindIn(image);

        Assert.NotNull(header);
        Assert.Equal("STANDUPC", header!.Product);
        Assert.Equal(4, header.Major);
        Assert.Equal(5, header.Minor);
        Assert.Equal(6, header.Patch);
        Assert.Equal(1234u, header.ImageSize);
        Assert.Equal(ConventionOffset, header.Offset);
        Assert.Equal("v4.5.6", header.VersionText);
    }

    [Fact]
    public void FindIn_HeaderAtOtherAlignedOffset_StillFound()
    {
        // A different MCU family's vector table ends elsewhere — the scan,
        // not a hard offset, is the contract.
        byte[] image = Embed(MakeHeader(), 0x2C0, 2048);

        ImageHeader? header = ImageHeader.FindIn(image);

        Assert.NotNull(header);
        Assert.Equal(0x2C0, header!.Offset);
    }

    [Fact]
    public void FindIn_NoMagic_ReturnsNull()
    {
        Assert.Null(ImageHeader.FindIn(new byte[2048]));
    }

    [Fact]
    public void FindIn_HeaderBeyondScanLimit_ReturnsNull()
    {
        byte[] image = Embed(MakeHeader(), ImageHeader.ScanLimit, 4096);

        Assert.Null(ImageHeader.FindIn(image));
    }

    [Fact]
    public void FindIn_UnalignedMagic_NotFound()
    {
        // Only word-aligned offsets are scanned; a shifted copy (e.g. the magic
        // string embedded inside other data) is not treated as a header.
        byte[] image = Embed(MakeHeader(), 0x18A, 2048);

        Assert.Null(ImageHeader.FindIn(image));
    }

    [Fact]
    public void FindIn_ImageTooSmallForHeader_ReturnsNull()
    {
        Assert.Null(ImageHeader.FindIn(new byte[8]));
    }

    [Fact]
    public void FindIn_StampedSelfCrc_Accepted()
    {
        byte[] image = Embed(MakeHeader(selfCrc: true), ConventionOffset, 2048);

        Assert.NotNull(ImageHeader.FindIn(image));
    }

    [Fact]
    public void FindIn_WrongSelfCrc_ReturnsNull()
    {
        byte[] image = Embed(MakeHeader(crc: 0xDEADBEEF), ConventionOffset, 2048);

        Assert.Null(ImageHeader.FindIn(image));
    }

    [Fact]
    public void FindIn_ProductPadding_IsTrimmed()
    {
        byte[] image = Embed(MakeHeader("LED "), ConventionOffset, 2048);

        Assert.Equal("LED", ImageHeader.FindIn(image)!.Product);
    }

    [Fact]
    public void FindIn_UnprintableProduct_ReturnsNull()
    {
        byte[] header = MakeHeader();
        header[9] = 0x01; // control character inside the product tag

        Assert.Null(ImageHeader.FindIn(Embed(header, ConventionOffset, 2048)));
    }

    [Fact]
    public void FindIn_EmptyProduct_ReturnsNull()
    {
        Assert.Null(ImageHeader.FindIn(Embed(MakeHeader(""), ConventionOffset, 2048)));
    }

    [Theory]
    [InlineData(1, 2, 3, 1, 2, 3, 0)]   // equal
    [InlineData(1, 2, 3, 1, 2, 4, -1)]  // older patch
    [InlineData(1, 3, 0, 1, 2, 9, 1)]   // newer minor beats higher patch
    [InlineData(2, 0, 0, 1, 9, 9, 1)]   // newer major beats everything
    public void CompareVersionTo_OrdersByMajorMinorPatch(
        ushort aMaj, ushort aMin, ushort aPat,
        ushort bMaj, ushort bMin, ushort bPat, int expectedSign)
    {
        ImageHeader a = ImageHeader.FindIn(Embed(MakeHeader(major: aMaj, minor: aMin, patch: aPat), 0, 64))!;
        ImageHeader b = ImageHeader.FindIn(Embed(MakeHeader(major: bMaj, minor: bMin, patch: bPat), 0, 64))!;

        Assert.Equal(expectedSign, Math.Sign(a.CompareVersionTo(b)));
    }
}
