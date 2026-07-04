// ----------------------------------------------------------------------------
//  CliOptions.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Parsed command-line options shared by all CLI verbs. Populated by
//  CliParser; each command validates the subset it requires.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Cli;

/// <summary>
/// Options parsed from the command line. Nullable members are "not supplied";
/// each command checks the ones it needs and reports a usage error otherwise.
/// </summary>
public sealed class CliOptions
{
    /// <summary>Serial port name (e.g. "COM3", "/dev/ttyUSB0").</summary>
    public string? Port { get; set; }

    /// <summary>Firmware <c>.bin</c> path (factory-reset, upload, verify).</summary>
    public string? File { get; set; }

    /// <summary>Output file path (read).</summary>
    public string? Output { get; set; }

    /// <summary>Memory address (read, go, optional image start for write verbs).</summary>
    public uint? Address { get; set; }

    /// <summary>Byte count (read).</summary>
    public int? Length { get; set; }

    /// <summary>Serial baud rate. The ST bootloader auto-detects via 0x7F.</summary>
    public int Baud { get; set; } = 115200;

    /// <summary>Number of 0x7F sync attempts before giving up on a silent device.</summary>
    public int Retries { get; set; } = 5;

    /// <summary>Per-command serial read/write timeout in milliseconds.</summary>
    public int TimeoutMs { get; set; } = 2000;

    /// <summary>Suppress the final Go (0x21); leave the device in bootloader mode.</summary>
    public bool NoGo { get; set; }

    /// <summary>Emit machine-readable JSON Lines instead of a human progress bar.</summary>
    public bool Json { get; set; }
}
