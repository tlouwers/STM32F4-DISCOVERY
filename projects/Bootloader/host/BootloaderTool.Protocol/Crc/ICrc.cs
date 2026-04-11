// ----------------------------------------------------------------------------
//  ICrc.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Interface for CRC computation.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Crc;

/// <summary>
/// Generic interface for CRC computation.
/// </summary>
public interface ICrc
{
    /// <summary>
    /// Computes the CRC over the given byte buffer.
    /// </summary>
    /// <param name="data">Input bytes.</param>
    /// <returns>CRC result.</returns>
    uint Compute(ReadOnlySpan<byte> data);
}
