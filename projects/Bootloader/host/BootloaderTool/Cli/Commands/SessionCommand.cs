// ----------------------------------------------------------------------------
//  SessionCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Shared driver for the verbs backed by FactoryResetSession (factory-reset and
//  upload). Loads the image, opens the port, wires progress/log to the console,
//  and maps the outcome to an exit code. The two verbs differ only in whether
//  the final Go runs and whether the chip-ID guard is enforced.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Cli.Commands;

/// <summary>Common erase/write/verify driver for the FactoryResetSession verbs.</summary>
internal static class SessionCommand
{
    /// <summary>STM32F40x/41x device ID used as the factory-reset chip guard.</summary>
    private const ushort Stm32F4ChipId = 0x0413;

    /// <summary>Runs a FactoryResetSession with the given Go / chip-guard policy.</summary>
    /// <param name="options">Parsed CLI options (needs -f and -p).</param>
    /// <param name="context">Ambient writers and serial seam.</param>
    /// <param name="runGo">Issue Go (0x21) after a successful verify.</param>
    /// <param name="guardChipId">Fail if the device ID is not STM32F40x/41x.</param>
    public static async Task<int> RunAsync(
        CliOptions options, CliContext context, bool runGo, bool guardChipId)
    {
        if (string.IsNullOrWhiteSpace(options.File))
            return CliResult.UsageError(context, "no firmware image specified (use -f <image.bin>)");

        FirmwareImage image;
        try
        {
            image = new FirmwareImage(options.File, options.Address ?? FirmwareImage.DefaultStartAddress);
        }
        catch (Exception ex)
        {
            return CliResult.Fail(context, $"cannot load firmware '{options.File}': {ex.Message}");
        }

        ISerial? serial = context.OpenPort(options, out string? error);
        if (serial is null)
            return CliResult.UsageOrFail(context, error!, options.Port is null);

        using (serial)
        {
            var resetOptions = new FactoryResetOptions
            {
                RunGo          = runGo,
                ExpectedChipId = guardChipId ? Stm32F4ChipId : null,
                SyncAttempts   = options.Retries,
            };

            var session  = new FactoryResetSession(serial, resetOptions);
            var progress = new ConsoleProgress(context.Out, options.Json);
            session.Log      += progress.OnLog;
            session.Progress += progress.OnProgress;

            try
            {
                await session.RunAsync(image);

                if (!options.Json)
                    context.Out.WriteLine(runGo ? "Done — device booted." : "Done — image written and verified.");

                return CliResult.Success;
            }
            catch (Exception ex)
            {
                progress.OnError(Describe(ex));
                return CliResult.Failure;
            }
        }
    }

    private static string Describe(Exception ex) => ex switch
    {
        NackException nack          => $"device rejected command 0x{nack.Command:X2} (NACK).",
        BootloaderTimeoutException  => $"timed out waiting for the device: {ex.Message}",
        ConnectionLostException     => "connection lost during transfer (" + ex.Message +
                                       "). The bootloader cannot resume a partial write — re-enter the " +
                                       "bootloader (RESET, then the blue button) and run the command again.",
        ChecksumMismatchException c => $"verification failed after retries: expected 0x{c.Expected:X8}, device 0x{c.Actual:X8}.",
        InvalidOperationException   => ex.Message,
        _                           => ex.Message,
    };
}
