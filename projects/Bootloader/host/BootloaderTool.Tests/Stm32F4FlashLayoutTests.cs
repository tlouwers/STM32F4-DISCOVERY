// ----------------------------------------------------------------------------
//  Stm32F4FlashLayoutTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for Stm32F4FlashLayout sector mapping and range validation.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using Xunit;

namespace BootloaderTool.Tests;

public class Stm32F4FlashLayoutTests
{
    [Fact]
    public void SectorsForRange_WithinSectorZero_ReturnsSectorZero()
    {
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x08000000, 100);

        Assert.Equal(new ushort[] { 0 }, sectors);
    }

    [Fact]
    public void SectorsForRange_SpanningSectorBoundary_ReturnsBothSectors()
    {
        // Sector 0 is 16 KB; 16 KB + 10 bytes spills into sector 1.
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x08000000, 16 * 1024 + 10);

        Assert.Equal(new ushort[] { 0, 1 }, sectors);
    }

    [Fact]
    public void SectorsForRange_ExactlyFillsSectorZero_ReturnsOnlySectorZero()
    {
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x08000000, 16 * 1024);

        Assert.Equal(new ushort[] { 0 }, sectors);
    }

    [Fact]
    public void SectorsForRange_AtSectorFiveStart_ReturnsSectorFive()
    {
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x08020000, 100);

        Assert.Equal(new ushort[] { 5 }, sectors);
    }

    [Fact]
    public void SectorsForRange_LastByteOfFlash_ReturnsSectorEleven()
    {
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x080FFFFF, 1);

        Assert.Equal(new ushort[] { 11 }, sectors);
    }

    [Fact]
    public void SectorsForRange_FullFlash_ReturnsAllSectors()
    {
        ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(0x08000000, (int)Stm32F4FlashLayout.FlashSize);

        Assert.Equal(Enumerable.Range(0, 12).Select(i => (ushort)i).ToArray(), sectors);
    }

    [Fact]
    public void SectorsForRange_ZeroLength_Throws()
    {
        Assert.Throws<ArgumentOutOfRangeException>(
            () => Stm32F4FlashLayout.SectorsForRange(0x08000000, 0));
    }

    [Fact]
    public void SectorsForRange_BelowFlashBase_Throws()
    {
        Assert.Throws<ArgumentOutOfRangeException>(
            () => Stm32F4FlashLayout.SectorsForRange(0x07FFFFFF, 4));
    }

    [Fact]
    public void SectorsForRange_ExceedsFlashEnd_Throws()
    {
        Assert.Throws<ArgumentOutOfRangeException>(
            () => Stm32F4FlashLayout.SectorsForRange(0x08000000, (int)Stm32F4FlashLayout.FlashSize + 1));
    }
}
