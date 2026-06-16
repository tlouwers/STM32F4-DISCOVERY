// ----------------------------------------------------------------------------
//  InfoCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "info" verb — connect to a device in bootloader mode and report chip ID,
//  protocol version, and supported commands.
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

/// <summary>Queries a connected bootloader for its identity and capabilities.</summary>
public sealed class InfoCommand : ICliCommand
{
    public string Name    => "info";
    public string Summary => "Query a device in bootloader mode (chip ID, version, commands).";
    public string Usage   => "info -p <port> [--baud <n>] [--timeout <ms>] [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
    {
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
                GetResult get = client.Get();
                ushort id     = client.GetId();

                string version = $"{get.ProtocolVersion >> 4}.{get.ProtocolVersion & 0x0F}";
                string commands = string.Join(" ", get.SupportedCommands.Select(c => $"0x{c:X2}"));

                if (options.Json)
                {
                    context.Out.WriteLine(System.Text.Json.JsonSerializer.Serialize(new
                    {
                        chipId        = $"0x{id:X4}",
                        chipName      = ChipName(id),
                        protocol      = version,
                        commandCount  = get.SupportedCommands.Length,
                        commands      = get.SupportedCommands.Select(c => $"0x{c:X2}"),
                    }));
                }
                else
                {
                    context.Out.WriteLine($"Chip ID : 0x{id:X4} ({ChipName(id)})");
                    context.Out.WriteLine($"Protocol: v{version}");
                    context.Out.WriteLine($"Commands: {get.SupportedCommands.Length} supported");
                    context.Out.WriteLine($"          {commands}");
                }

                return Task.FromResult(0);
            }
            catch (Exception ex)
            {
                return Task.FromResult(CliResult.Protocol(context, ex));
            }
        }
    }

    /// <summary>Maps a few common STM32 device IDs to a human-readable family.</summary>
    private static string ChipName(ushort id) => id switch
    {
        0x0413 => "STM32F405/407/415/417",
        0x0419 => "STM32F42x/43x",
        0x0431 => "STM32F411",
        0x0441 => "STM32F412",
        0x0421 => "STM32F446",
        _      => "unknown",
    };
}
