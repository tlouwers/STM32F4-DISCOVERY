// ----------------------------------------------------------------------------
//  FactoryResetOptions.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Tunable parameters for the factory reset orchestrator: timeouts, retry
//  counts, chip-ID guard, and whether to jump to the application at the end.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Tunable parameters for <see cref="FactoryResetSession"/>.
/// </summary>
public sealed class FactoryResetOptions
{
    /// <summary>
    /// Issue the Go (0x21) command after a successful verify, jumping to the
    /// application. Disable to leave the device in bootloader mode.
    /// </summary>
    public bool RunGo { get; set; } = true;

    /// <summary>
    /// If set, the chip ID returned by Get ID (0x02) must match this value or
    /// the session fails. STM32F40x/41x report 0x0413.
    /// </summary>
    public ushort? ExpectedChipId { get; set; } = 0x0413;

    /// <summary>Timeout for the Extended Erase ACK (erase is slow). Default 60 s.</summary>
    public int EraseTimeoutMs { get; set; } = 60000;

    /// <summary>
    /// Maximum number of erase+write+verify attempts before giving up on a
    /// persistent CRC mismatch. Default 3.
    /// </summary>
    public int MaxWriteAttempts { get; set; } = 3;

    /// <summary>Number of 0x7F sync attempts on initial connect. Default 5.</summary>
    public int SyncAttempts { get; set; } = 5;

    /// <summary>Delay between initial sync attempts. Default 1 s.</summary>
    public int SyncDelayMs { get; set; } = 1000;
}
