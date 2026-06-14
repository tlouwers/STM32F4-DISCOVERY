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
