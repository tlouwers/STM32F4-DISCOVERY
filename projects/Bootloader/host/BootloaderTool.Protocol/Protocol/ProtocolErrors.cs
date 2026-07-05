// ----------------------------------------------------------------------------
//  ProtocolErrors.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Single mapping from protocol exceptions to plain-language user messages,
//  shared by the GUI and both CLI paths so the wording cannot drift.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Turns a protocol exception into a plain-language message. The one shared
/// mapping for every front-end; only the retry phrasing differs per caller.
/// </summary>
public static class ProtocolErrors
{
    /// <summary>Maps a protocol exception to a user-facing message.</summary>
    /// <param name="ex">The exception to describe.</param>
    /// <param name="retryHint">
    /// How the user retries in the calling front-end, spliced into the
    /// connection-lost guidance (e.g. "click Retry" in the GUI, "run the
    /// command again" in the CLI).
    /// </param>
    /// <returns>A plain-language description of the failure.</returns>
    public static string Describe(Exception ex, string retryHint = "run the command again") => ex switch
    {
        NackException nack          => $"Device rejected command 0x{nack.Command:X2} (NACK).",
        BootloaderTimeoutException  => $"Timed out waiting for the device: {ex.Message}",
        ConnectionLostException     => $"Connection lost during transfer ({ex.Message}). The bootloader " +
                                       "cannot resume a partial write — re-enter the bootloader (RESET, then " +
                                       $"the blue button) and {retryHint}.",
        ChecksumMismatchException c => $"Verification failed after retries: expected 0x{c.Expected:X8}, device 0x{c.Actual:X8}.",
        _                           => ex.Message,
    };
}
