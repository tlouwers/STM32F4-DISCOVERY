// ----------------------------------------------------------------------------
//  VerifyCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "verify" verb — compute the device CRC over the region a firmware image
//  occupies and compare it against the image's own CRC32.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Crc;
using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Cli.Commands;

/// <summary>Compares the device's CRC over a region against a firmware image.</summary>
public sealed class VerifyCommand : ICliCommand
{
    public string Name    => "verify";
    public string Summary => "Compare the device CRC over a region against a firmware image.";
    public string Usage   => "verify -p <port> -f <image.bin> [--addr <hex>] [--baud <n>] [--timeout <ms>] [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
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

        ISerial? serial = context.OpenPort(options, out string? error);
        if (serial is null)
            return Task.FromResult(CliResult.UsageOrFail(context, error!, options.Port is null));

        using (serial)
        {
            var client = new An3155Client(serial, options.Retries);
            if (!client.SyncWithRetries())
                return Task.FromResult(CliResult.NoSync(context));

            try
            {
                // The STM32F4 ROM bootloader has no Get-Checksum (0xA1) command;
                // fall back to reading the region back and CRC-comparing it.
                GetResult get = client.Get();
                bool hasChecksum = get.SupportedCommands.Contains(An3155Constants.CmdGetChecksum);

                uint deviceCrc = hasChecksum
                    ? client.GetChecksum(image.StartAddress, image.WordCount)
                    : new Crc32().Compute(client.ReadRegion(image.StartAddress, image.Size));
                bool match = deviceCrc == image.Crc32;

                if (options.Json)
                {
                    context.Out.WriteLine(System.Text.Json.JsonSerializer.Serialize(new
                    {
                        match,
                        expected = $"0x{image.Crc32:X8}",
                        device   = $"0x{deviceCrc:X8}",
                    }));
                }
                else if (match)
                {
                    context.Out.WriteLine($"CRC32 OK (0x{deviceCrc:X8})");
                }
                else
                {
                    context.Out.WriteLine($"CRC32 mismatch: image 0x{image.Crc32:X8}, device 0x{deviceCrc:X8}");
                }

                return Task.FromResult(match ? CliResult.Success : CliResult.Failure);
            }
            catch (Exception ex)
            {
                return Task.FromResult(CliResult.Protocol(context, ex));
            }
        }
    }
}
