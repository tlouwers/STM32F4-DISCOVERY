// ----------------------------------------------------------------------------
//  ISerialPortEnumerator.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Abstraction over host serial-port discovery, so the watcher can be unit
//  tested with a scripted enumerator instead of real hardware.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// Enumerates the serial ports currently present on the host. Implementations
/// must be safe to call repeatedly from a background poll loop.
/// </summary>
public interface ISerialPortEnumerator
{
    /// <summary>
    /// Returns the serial ports currently visible to the host. May throw if the
    /// underlying OS enumeration fails transiently (e.g. a device being removed);
    /// callers in a poll loop should treat a throw as "no change this tick".
    /// </summary>
    IReadOnlyList<SerialPortInfo> Enumerate();
}
