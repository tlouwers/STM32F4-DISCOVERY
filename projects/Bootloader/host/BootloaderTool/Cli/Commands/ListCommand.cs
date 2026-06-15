// ----------------------------------------------------------------------------
//  ListCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "list" verb — enumerate serial ports and probe each for an ST bootloader
//  by sending 0x7F and watching for an ACK.
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

/// <summary>Lists serial ports and flags those answering the bootloader sync.</summary>
public sealed class ListCommand : ICliCommand
{
    public string Name    => "list";
    public string Summary => "List serial ports and probe each for an ST bootloader.";
    public string Usage   => "list [--baud <n>] [--timeout <ms>]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
    {
        IReadOnlyList<string> ports = context.PortNames();
        if (ports.Count == 0)
        {
            context.Out.WriteLine("No serial ports found.");
            return Task.FromResult(0);
        }

        var config = new SerialConfig { BaudRate = options.Baud, TimeoutMs = options.TimeoutMs };

        context.Out.WriteLine($"{"PORT",-16}  STATUS");
        foreach (string port in ports)
        {
            string status = Probe(context, port, config);
            context.Out.WriteLine($"{port,-16}  {status}");
        }

        return Task.FromResult(0);
    }

    private static string Probe(CliContext context, string port, SerialConfig config)
    {
        ISerial serial = context.SerialFactory();
        try
        {
            if (!serial.Open(port, config))
                return "unavailable";

            return new An3155Client(serial).Sync() ? "bootloader (ACK)" : "no response";
        }
        catch (Exception)
        {
            return "error";
        }
        finally
        {
            serial.Dispose();
        }
    }
}
