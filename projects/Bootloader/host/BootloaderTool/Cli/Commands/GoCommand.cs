// ----------------------------------------------------------------------------
//  GoCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "go" verb — instruct a device in bootloader mode to jump to an address.
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

/// <summary>Jumps to an application address via the Go (0x21) command.</summary>
public sealed class GoCommand : ICliCommand
{
    public string Name    => "go";
    public string Summary => "Jump to an application address (Go 0x21).";
    public string Usage   => "go -p <port> [--addr <hex>] [--baud <n>] [--timeout <ms>]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
    {
        ISerial? serial = context.OpenPort(options, out string? error);
        if (serial is null)
            return Task.FromResult(CliResult.UsageOrFail(context, error!, options.Port is null));

        using (serial)
        {
            var client = new An3155Client(serial, options.Retries);
            if (!client.Sync())
                return Task.FromResult(CliResult.NoSync(context));

            uint address = options.Address ?? FirmwareImage.DefaultStartAddress;

            try
            {
                client.Go(address);
                context.Out.WriteLine($"Jumped to 0x{address:X8}.");
                return Task.FromResult(CliResult.Success);
            }
            catch (Exception ex)
            {
                return Task.FromResult(CliResult.Protocol(context, ex));
            }
        }
    }
}
