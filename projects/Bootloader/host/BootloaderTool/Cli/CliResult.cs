// ----------------------------------------------------------------------------
//  CliResult.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Shared exit-code helpers for CLI commands. Centralises the mapping from a
//  failure condition (usage error, no sync, protocol exception) to a friendly
//  message on stderr plus the matching process exit code.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;

namespace BootloaderTool.Cli;

/// <summary>
/// Exit-code constants and helpers shared by commands. Convention: 0 success,
/// 1 runtime failure, 2 usage error.
/// </summary>
public static class CliResult
{
    /// <summary>Successful completion.</summary>
    public const int Success = 0;

    /// <summary>A runtime failure (port open, protocol, I/O).</summary>
    public const int Failure = 1;

    /// <summary>A usage error (missing or invalid options).</summary>
    public const int Usage = 2;

    /// <summary>Reports a usage error and returns the usage exit code.</summary>
    public static int UsageError(CliContext context, string message)
    {
        context.Err.WriteLine($"error: {message}");
        return Usage;
    }

    /// <summary>Reports a runtime failure and returns the failure exit code.</summary>
    public static int Fail(CliContext context, string message)
    {
        context.Err.WriteLine($"error: {message}");
        return Failure;
    }

    /// <summary>
    /// Picks usage vs. runtime failure for an open-port error: a missing port
    /// name is the user's mistake (usage), anything else is a runtime failure.
    /// </summary>
    public static int UsageOrFail(CliContext context, string message, bool isUsage)
        => isUsage ? UsageError(context, message) : Fail(context, message);

    /// <summary>Reports the standard "no sync" failure.</summary>
    public static int NoSync(CliContext context) => Fail(context,
        "bootloader did not respond to sync (0x7F). Check wiring, parity (must be 8E1), " +
        "and that the device is in bootloader mode.");

    /// <summary>Maps a protocol exception to a friendly message and failure code.</summary>
    public static int Protocol(CliContext context, Exception ex)
    {
        string message = ex switch
        {
            NackException nack          => $"device rejected command 0x{nack.Command:X2} (NACK).",
            BootloaderTimeoutException  => $"timed out waiting for the device: {ex.Message}",
            ConnectionLostException     => $"serial connection lost: {ex.Message}",
            ChecksumMismatchException c => $"verification failed: expected 0x{c.Expected:X8}, device 0x{c.Actual:X8}.",
            _                           => ex.Message,
        };
        return Fail(context, message);
    }
}
