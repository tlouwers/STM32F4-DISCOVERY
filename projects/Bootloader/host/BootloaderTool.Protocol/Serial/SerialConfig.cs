// ----------------------------------------------------------------------------
//  SerialConfig.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Configuration for serial port communication.
//  The AN3155 bootloader requires 8E1 (8 data bits, even parity, 1 stop bit).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

using System.IO.Ports;

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// Configuration for serial port communication.
/// Defaults match AN3155 requirements: 115200 baud, 8E1.
/// </summary>
public sealed class SerialConfig
{
    public int      BaudRate  { get; set; } = 115200;
    public int      DataBits  { get; set; } = 8;
    public Parity   Parity    { get; set; } = Parity.Even;
    public StopBits StopBits  { get; set; } = StopBits.One;
    public int      TimeoutMs { get; set; } = 2000;
}
