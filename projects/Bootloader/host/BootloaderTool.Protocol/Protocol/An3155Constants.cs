// ----------------------------------------------------------------------------
//  An3155Constants.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Constants for the AN3155 USART bootloader protocol.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// AN3155 USART bootloader protocol constants.
/// </summary>
public static class An3155Constants
{
    // Framing
    public const byte SyncByte = 0x7F;
    public const byte Ack      = 0x79;
    public const byte Nack     = 0x1F;

    // Commands
    public const byte CmdGet           = 0x00;
    public const byte CmdGetVersion    = 0x01;
    public const byte CmdGetId         = 0x02;
    public const byte CmdReadMemory    = 0x11;
    public const byte CmdGo            = 0x21;
    public const byte CmdWriteMemory   = 0x31;
    public const byte CmdErase         = 0x43;
    public const byte CmdExtendedErase = 0x44;
    public const byte CmdWriteProtect  = 0x63;
    public const byte CmdWriteUnprotect = 0x73;
    public const byte CmdReadProtect   = 0x82;
    public const byte CmdReadUnprotect = 0x92;
    public const byte CmdGetChecksum   = 0xA1;

    // Limits
    public const int MaxWriteBytes = 256;
    public const int MaxReadBytes  = 256;
}
