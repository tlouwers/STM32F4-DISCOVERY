// ----------------------------------------------------------------------------
//  ReadCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "read" verb — read a memory region from a device in bootloader mode in
//  256-byte chunks and write it to a file.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;

namespace BootloaderTool.Cli.Commands;

/// <summary>Reads a memory region from the device and writes it to a file.</summary>
public sealed class ReadCommand : ICliCommand
{
    public string Name    => "read";
    public string Summary => "Read a memory region to a file.";
    public string Usage   => "read -p <port> --addr <hex> --len <n> -o <file> [--baud <n>] [--timeout <ms>] [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
    {
        if (options.Address is null)
            return Task.FromResult(CliResult.UsageError(context, "no address specified (use --addr <hex>)"));
        if (options.Length is null)
            return Task.FromResult(CliResult.UsageError(context, "no length specified (use --len <n>)"));
        if (string.IsNullOrWhiteSpace(options.Output))
            return Task.FromResult(CliResult.UsageError(context, "no output file specified (use -o <file>)"));

        return ClientCommand.Run(options, context, client =>
        {
            var  progress = new ConsoleProgress(context.Out, options.Json);
            uint address  = options.Address.Value;
            int  total    = options.Length.Value;

            byte[] buffer = client.ReadRegion(address, total,
                (done, len) => progress.OnProgress("Reading", (uint)done, (uint)len));

            File.WriteAllBytes(options.Output!, buffer);
            progress.OnLog("info", $"Read {total} bytes to {options.Output}.");
            return CliResult.Success;
        });
    }
}
