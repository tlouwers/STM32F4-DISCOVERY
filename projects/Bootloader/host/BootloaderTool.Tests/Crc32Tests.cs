// ----------------------------------------------------------------------------
//  Crc32Tests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for CRC-32/MPEG-2 (matching STM32F4 hardware CRC peripheral).
//  Reference values from the C++ GoogleTest suite (TestCrc32.cpp).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Crc;
using Xunit;

namespace BootloaderTool.Tests;

public class Crc32Tests
{
    private readonly Crc32 _crc = new();

    [Fact]
    public void SingleWord_0x00000000()
    {
        byte[] data = { 0x00, 0x00, 0x00, 0x00 };
        Assert.Equal(0xC704DD7Bu, _crc.Compute(data));
    }

    [Fact]
    public void SingleWord_0xFFFFFFFF()
    {
        byte[] data = { 0xFF, 0xFF, 0xFF, 0xFF };
        Assert.Equal(0x00000000u, _crc.Compute(data));
    }

    [Fact]
    public void SingleWord_0x12345678()
    {
        byte[] data = { 0x12, 0x34, 0x56, 0x78 };
        Assert.Equal(0xDF8A8A2Bu, _crc.Compute(data));
    }

    [Fact]
    public void TwoWords()
    {
        byte[] data = {
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x02
        };
        Assert.Equal(0x298BE7BAu, _crc.Compute(data));
    }

    [Fact]
    public void AscendingBytes_1_to_8()
    {
        byte[] data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
        Assert.Equal(0x140B8DD8u, _crc.Compute(data));
    }

    [Fact]
    public void EmptyInput_ReturnsInitValue()
    {
        Assert.Equal(0xFFFFFFFFu, _crc.Compute(ReadOnlySpan<byte>.Empty));
    }

    [Fact]
    public void ComputeTwice_SameResult()
    {
        byte[] data = { 0x12, 0x34, 0x56, 0x78 };
        uint first  = _crc.Compute(data);
        uint second = _crc.Compute(data);
        Assert.Equal(first, second);
    }

    [Fact]
    public void LargerBuffer_16Bytes()
    {
        byte[] data = {
            0xDE, 0xAD, 0xBE, 0xEF,
            0xCA, 0xFE, 0xBA, 0xBE,
            0x8B, 0xAD, 0xF0, 0x0D,
            0xFE, 0xED, 0xFA, 0xCE
        };
        Assert.Equal(0xF1B36814u, _crc.Compute(data));
    }
}
