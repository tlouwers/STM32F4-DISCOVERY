// ----------------------------------------------------------------------------
//  CliContext.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Ambient environment for CLI commands: output writers and injectable seams
//  for serial creation and port enumeration. Tests substitute a MockSerial
//  factory and a fixed port list so commands run without hardware.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.IO.Ports;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Cli;

/// <summary>
/// Holds the writers and creation seams a CLI command needs. The serial factory
/// and port-name provider are injectable so commands are testable against a
/// scripted mock; the defaults talk to real hardware via System.IO.Ports.
/// </summary>
public sealed class CliContext
{
    /// <summary>Standard output sink (progress, results).</summary>
    public required TextWriter Out { get; init; }

    /// <summary>Standard error sink (usage and runtime errors).</summary>
    public required TextWriter Err { get; init; }

    /// <summary>Creates a fresh, unopened serial port. Default: SerialPortAdapter.</summary>
    public Func<ISerial> SerialFactory { get; init; } = () => new SerialPortAdapter();

    /// <summary>Enumerates available port names. Default: OS-reported ports.</summary>
    public Func<IReadOnlyList<string>> PortNames { get; init; } = () => SerialPort.GetPortNames();

    /// <summary>Builds a context bound to the real console and hardware.</summary>
    public static CliContext Default() => new()
    {
        Out = Console.Out,
        Err = Console.Error,
    };

    /// <summary>
    /// Opens the serial port named by <paramref name="options"/> using an 8E1
    /// config at the requested baud and timeout. Returns the open port, or null
    /// with a populated <paramref name="error"/> on a missing name or open
    /// failure. The caller owns disposal of a non-null result.
    /// </summary>
    public ISerial? OpenPort(CliOptions options, out string? error)
    {
        if (string.IsNullOrWhiteSpace(options.Port))
        {
            error = "no serial port specified (use -p <port>)";
            return null;
        }

        var config = new SerialConfig
        {
            BaudRate  = options.Baud,
            TimeoutMs = options.TimeoutMs,
        };

        ISerial serial = SerialFactory();
        if (!serial.Open(options.Port, config))
        {
            serial.Dispose();
            error = $"could not open port '{options.Port}' (in use, missing, or access denied)";
            return null;
        }

        error = null;
        return serial;
    }
}
