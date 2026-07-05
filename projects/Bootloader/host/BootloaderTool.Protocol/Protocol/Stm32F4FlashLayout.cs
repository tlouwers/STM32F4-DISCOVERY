// ----------------------------------------------------------------------------
//  Stm32F4FlashLayout.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Flash sector geometry for the STM32F407VG (1 MB, sectors 0-11). Maps an
//  address range to the set of sectors that must be erased to cover it.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// STM32F407VG flash sector geometry (1 MB main memory, sectors 0-11).
/// </summary>
public static class Stm32F4FlashLayout
{
    /// <summary>Base address of the main flash memory.</summary>
    public const uint FlashBase = 0x08000000;

    /// <summary>Total main flash size (1 MB).</summary>
    public const uint FlashSize = 0x00100000;

    // Sector sizes in order (sectors 0-11). The start address of each sector is
    // the running sum of the preceding sector sizes added to FlashBase.
    private static readonly uint[] SectorSizes =
    {
        16 * 1024,  // 0
        16 * 1024,  // 1
        16 * 1024,  // 2
        16 * 1024,  // 3
        64 * 1024,  // 4
        128 * 1024, // 5
        128 * 1024, // 6
        128 * 1024, // 7
        128 * 1024, // 8
        128 * 1024, // 9
        128 * 1024, // 10
        128 * 1024, // 11
    };

    /// <summary>Number of sectors in main flash.</summary>
    public static int SectorCount => SectorSizes.Length;

    /// <summary>
    /// Returns the sector numbers that cover the given address range. The range
    /// must lie entirely within main flash.
    /// </summary>
    /// <param name="startAddress">Range start (need not be sector-aligned).</param>
    /// <param name="length">Range length in bytes (must be &gt; 0).</param>
    /// <returns>Ascending list of sector numbers overlapping the range.</returns>
    public static ushort[] SectorsForRange(uint startAddress, int length)
    {
        if (length <= 0)
            throw new ArgumentOutOfRangeException(nameof(length), "Length must be positive.");
        if (startAddress < FlashBase)
            throw new ArgumentOutOfRangeException(nameof(startAddress),
                $"Address 0x{startAddress:X8} is below flash base 0x{FlashBase:X8}.");

        // Use a long for the end to avoid uint overflow at the top of flash.
        long rangeStart = startAddress;
        long rangeEnd   = rangeStart + length; // exclusive

        if (rangeEnd > FlashBase + FlashSize)
            throw new ArgumentOutOfRangeException(nameof(length),
                $"Range 0x{startAddress:X8}..0x{rangeEnd:X8} exceeds flash end 0x{FlashBase + FlashSize:X8}.");

        var sectors = new List<ushort>();
        long sectorStart = FlashBase;
        for (int i = 0; i < SectorSizes.Length; i++)
        {
            long sectorEnd = sectorStart + SectorSizes[i]; // exclusive

            // Overlap test between [rangeStart, rangeEnd) and [sectorStart, sectorEnd).
            if (rangeStart < sectorEnd && rangeEnd > sectorStart)
                sectors.Add((ushort)i);

            sectorStart = sectorEnd;
        }

        return sectors.ToArray();
    }
}
