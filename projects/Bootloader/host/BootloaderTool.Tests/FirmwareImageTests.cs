// ----------------------------------------------------------------------------
//  FirmwareImageTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for FirmwareImage: loading, size/CRC metadata, word count, and
//  argument validation.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Crc;
using BootloaderTool.Protocol.Protocol;
using Xunit;

namespace BootloaderTool.Tests;

public class FirmwareImageTests
{
    [Fact]
    public void Constructor_NullData_ThrowsArgumentNull()
    {
        Assert.Throws<ArgumentNullException>(() => new FirmwareImage((byte[])null!));
    }

    [Fact]
    public void Constructor_EmptyData_ThrowsArgument()
    {
        Assert.Throws<ArgumentException>(() => new FirmwareImage(Array.Empty<byte>()));
    }

    [Fact]
    public void Constructor_DefaultStartAddress_IsFlashBase()
    {
        var image = new FirmwareImage(new byte[] { 0x01, 0x02, 0x03, 0x04 });

        Assert.Equal(0x08000000u, image.StartAddress);
    }

    [Fact]
    public void Constructor_CustomStartAddress_IsStored()
    {
        var image = new FirmwareImage(new byte[] { 0x01 }, 0x08020000);

        Assert.Equal(0x08020000u, image.StartAddress);
    }

    [Fact]
    public void Size_MatchesDataLength()
    {
        var image = new FirmwareImage(new byte[260]);

        Assert.Equal(260, image.Size);
    }

    [Fact]
    public void Crc32_MatchesCrc32Class()
    {
        byte[] data = { 0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78 };
        uint expected = new Crc32().Compute(data);

        var image = new FirmwareImage(data);

        Assert.Equal(expected, image.Crc32);
    }

    [Theory]
    [InlineData(1, 1u)]
    [InlineData(4, 1u)]
    [InlineData(5, 2u)]
    [InlineData(8, 2u)]
    [InlineData(260, 65u)]
    public void WordCount_RoundsUpToWholeWords(int size, uint expectedWords)
    {
        var image = new FirmwareImage(new byte[size]);

        Assert.Equal(expectedWords, image.WordCount);
    }

    [Fact]
    public void VectorTable_ParsedLittleEndian()
    {
        // SP = 0x20020000 (top of SRAM), Reset = 0x08000199 (flash, Thumb bit).
        byte[] data = { 0x00, 0x00, 0x02, 0x20, 0x99, 0x01, 0x00, 0x08 };

        var image = new FirmwareImage(data);

        Assert.Equal(0x20020000u, image.InitialStackPointer);
        Assert.Equal(0x08000199u, image.ResetHandler);
    }

    [Fact]
    public void VectorTable_ImageSmallerThanEightBytes_ReadsAsZero()
    {
        var image = new FirmwareImage(new byte[] { 0xFF, 0xFF, 0xFF, 0xFF });

        Assert.Equal(0u, image.InitialStackPointer);
        Assert.Equal(0u, image.ResetHandler);
    }

    [Fact]
    public void Header_PresentInImage_IsParsed()
    {
        byte[] image = new byte[1024];
        ImageHeader.Magic.CopyTo(image, 0x188);
        System.Text.Encoding.ASCII.GetBytes("F4DISCO1").CopyTo(image, 0x188 + 8);
        image[0x188 + 16] = 2; // major = 2 (little-endian u16)

        var firmware = new FirmwareImage(image);

        Assert.NotNull(firmware.Header);
        Assert.Equal("F4DISCO1", firmware.Header!.Product);
        Assert.Equal(2, firmware.Header.Major);
    }

    [Fact]
    public void Header_AbsentFromImage_IsNull()
    {
        Assert.Null(new FirmwareImage(new byte[1024]).Header);
    }

    [Fact]
    public void Constructor_FromFile_LoadsBytesAndComputesCrc()
    {
        byte[] data = Enumerable.Range(0, 64).Select(i => (byte)i).ToArray();
        string path = Path.GetTempFileName();
        try
        {
            File.WriteAllBytes(path, data);

            var image = new FirmwareImage(path);

            Assert.Equal(data.Length, image.Size);
            Assert.Equal(data, image.Data);
            Assert.Equal(new Crc32().Compute(data), image.Crc32);
        }
        finally
        {
            File.Delete(path);
        }
    }
}
