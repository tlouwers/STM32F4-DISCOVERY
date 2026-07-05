// ----------------------------------------------------------------------------
//  ISerial.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Interface for serial port communication.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// Generic interface for serial port communication.
/// Implementations handle OS-specific serial I/O. The ST AN3155 bootloader
/// protocol requires 8E1 (8 data bits, even parity, 1 stop bit).
/// </summary>
public interface ISerial : IDisposable
{
    /// <summary>Opens the serial port with the given configuration.</summary>
    /// <param name="portName">Port name (e.g. "COM3", "/dev/ttyUSB0").</param>
    /// <param name="config">Serial configuration.</param>
    /// <returns>True if the port was opened successfully.</returns>
    bool Open(string portName, SerialConfig config);

    /// <summary>Closes the serial port.</summary>
    void Close();

    /// <summary>Returns true if the port is currently open.</summary>
    bool IsOpen { get; }

    /// <summary>Writes bytes to the serial port.</summary>
    /// <param name="data">Bytes to send.</param>
    /// <returns>Number of bytes actually written, or -1 on error.</returns>
    int Write(ReadOnlySpan<byte> data);

    /// <summary>Reads bytes from the serial port (blocking up to timeout).</summary>
    /// <param name="buffer">Buffer to fill.</param>
    /// <returns>Number of bytes read, or -1 on error. Returns 0 on timeout.</returns>
    int Read(Span<byte> buffer);

    /// <summary>Changes the read/write timeout.</summary>
    /// <param name="timeoutMs">Timeout in milliseconds.</param>
    void SetTimeout(int timeoutMs);

    /// <summary>Currently configured read/write timeout in milliseconds.</summary>
    int TimeoutMs { get; }

    /// <summary>Discards any data sitting in the input buffer.</summary>
    void FlushInput();
}
