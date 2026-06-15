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
using BootloaderTool.Protocol.Serial;

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

        ISerial? serial = context.OpenPort(options, out string? error);
        if (serial is null)
            return Task.FromResult(CliResult.UsageOrFail(context, error!, options.Port is null));

        using (serial)
        {
            var client   = new An3155Client(serial, options.Retries);
            var progress = new ConsoleProgress(context.Out, options.Json);

            if (!client.Sync())
                return Task.FromResult(CliResult.NoSync(context));

            uint address  = options.Address.Value;
            int  total    = options.Length.Value;
            var  buffer   = new byte[total];
            int  readSoFar = 0;

            try
            {
                while (readSoFar < total)
                {
                    int chunk = Math.Min(An3155Constants.MaxReadBytes, total - readSoFar);
                    byte[] part = client.ReadMemory(address + (uint)readSoFar, chunk);
                    Array.Copy(part, 0, buffer, readSoFar, chunk);
                    readSoFar += chunk;
                    progress.OnProgress("Reading", (uint)readSoFar, (uint)total);
                }

                File.WriteAllBytes(options.Output, buffer);
                progress.OnLog("info", $"Read {total} bytes to {options.Output}.");
                return Task.FromResult(CliResult.Success);
            }
            catch (Exception ex)
            {
                return Task.FromResult(CliResult.Protocol(context, ex));
            }
        }
    }
}
