// ----------------------------------------------------------------------------
//  ClientCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Shared runner for verbs that talk to a synced bootloader: opens the port,
//  performs the sync handshake, hands a ready An3155Client to the verb body,
//  and maps protocol exceptions to the standard failure exit code.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Cli.Commands;

/// <summary>
/// Owns the open-port → sync → run → map-errors skeleton every device verb
/// shares, so each command contains only its own protocol work.
/// </summary>
internal static class ClientCommand
{
    /// <summary>Runs a synchronous verb body against a synced client.</summary>
    /// <param name="options">Parsed CLI options (needs -p).</param>
    /// <param name="context">Ambient writers and serial seam.</param>
    /// <param name="body">The verb's work; returns its exit code.</param>
    /// <returns>The verb's exit code, or the standard failure mapping.</returns>
    public static Task<int> Run(CliOptions options, CliContext context, Func<An3155Client, int> body)
        => RunAsync(options, context, client => Task.FromResult(body(client)));

    /// <summary>Runs an asynchronous verb body against a synced client.</summary>
    /// <param name="options">Parsed CLI options (needs -p).</param>
    /// <param name="context">Ambient writers and serial seam.</param>
    /// <param name="body">The verb's work; returns its exit code.</param>
    /// <returns>The verb's exit code, or the standard failure mapping.</returns>
    public static async Task<int> RunAsync(
        CliOptions options, CliContext context, Func<An3155Client, Task<int>> body)
    {
        ISerial? serial = context.OpenPort(options, out string? error);
        if (serial is null)
            return CliResult.UsageOrFail(context, error!, options.Port is null);

        using (serial)
        {
            // SyncWithRetries also accepts the NACK an already-armed bootloader
            // sends to a repeated 0x7F, where a bare Sync() would fail.
            var client = new An3155Client(serial);
            if (!client.SyncWithRetries(attempts: options.Retries))
                return CliResult.NoSync(context);

            try
            {
                return await body(client);
            }
            catch (Exception ex)
            {
                return CliResult.Protocol(context, ex);
            }
        }
    }
}
