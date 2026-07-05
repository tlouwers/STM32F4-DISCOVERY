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
    public static Task<int> RunAsync(
        CliOptions options, CliContext context, bool runGo, bool guardChipId)
    {
        if (string.IsNullOrWhiteSpace(options.File))
            return Task.FromResult(CliResult.UsageError(context, "no firmware image specified (use -f <image.bin>)"));

        FirmwareImage image;
        try
        {
            image = new FirmwareImage(options.File, options.Address ?? FirmwareImage.DefaultStartAddress);
        }
        catch (Exception ex)
        {
            return Task.FromResult(CliResult.Fail(context, $"cannot load firmware '{options.File}': {ex.Message}"));
        }

        // Image sanity — the same gates the GUI applies at selection. A blob
        // deliberately written elsewhere (e.g. data at a custom --addr) can be
        // pushed past them with --force.
        string? rejection = ImageCompatibility.CheckVectorTable(image)
                            ?? ImageCompatibility.CheckDeclaredSize(image);
        if (rejection is not null)
        {
            if (!options.Force)
                return Task.FromResult(CliResult.Fail(context, $"{rejection} Use --force to flash anyway."));
            context.Err.WriteLine($"warning: {rejection} Continuing (--force).");
        }

        return ClientCommand.RunAsync(options, context, async client =>
        {
            var progress = new ConsoleProgress(context.Out, options.Json);

            // Read the start of the target region and look for a "TLFWIMG1"
            // metadata header, so the product/downgrade gates can compare
            // against what the device currently runs. Best-effort: a
            // read-protected or erased device simply reports no header.
            ImageHeader? deviceHeader = null;
            try
            {
                byte[] head = client.ReadRegion(image.StartAddress, ImageHeader.ScanLimit);
                deviceHeader = ImageHeader.FindIn(head);
            }
            catch (Exception ex) when (ex is NackException || ex is BootloaderTimeoutException)
            {
                // Read Memory refused (e.g. RDP active) or timed out — treat as
                // unknown. Clear any partial response so later commands start clean.
                client.Serial.FlushInput();
            }

            string? gate = ImageCompatibility.CheckProduct(image.Header, deviceHeader)
                           ?? ImageCompatibility.CheckDowngrade(image.Header, deviceHeader);
            if (gate is not null)
            {
                if (!options.Force)
                    return CliResult.Fail(context, $"{gate} Use --force to flash anyway.");
                context.Err.WriteLine($"warning: {gate} Continuing (--force).");
            }

            var resetOptions = new FactoryResetOptions
            {
                RunGo          = runGo,
                ExpectedChipId = guardChipId ? Stm32F4ChipId : null,
                SyncAttempts   = options.Retries,
            };

            // The client is already synced (one-shot 0x7F consumed by the
            // runner), so hand it over and let the session skip its own sync.
            var session = new FactoryResetSession(client, resetOptions);
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
                progress.OnError(ProtocolErrors.Describe(ex));
                return CliResult.Failure;
            }
        });
    }
}
