// ----------------------------------------------------------------------------
//  An3155Exception.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Exceptions for AN3155 protocol errors.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Thrown when the bootloader responds with NACK.
/// </summary>
public class NackException : Exception
{
    public byte Command { get; }

    public NackException(byte command)
        : base($"Bootloader NACKed command 0x{command:X2}")
    {
        Command = command;
    }
}

/// <summary>
/// Thrown when the bootloader does not respond within the timeout.
/// </summary>
public class TimeoutException : Exception
{
    public TimeoutException(string message) : base(message) { }
}

/// <summary>
/// Thrown when the serial connection is lost.
/// </summary>
public class ConnectionLostException : Exception
{
    public ConnectionLostException(string message) : base(message) { }
}
