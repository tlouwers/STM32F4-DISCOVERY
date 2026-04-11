// ----------------------------------------------------------------------------
//  SerialPortAdapter.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  ISerial implementation backed by System.IO.Ports.SerialPort.
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
/// ISerial implementation using System.IO.Ports.SerialPort.
/// </summary>
public sealed class SerialPortAdapter : ISerial
{
    private SerialPort? _port;

    public bool IsOpen => _port?.IsOpen ?? false;

    public bool Open(string portName, SerialConfig config)
    {
        try
        {
            _port = new SerialPort(portName)
            {
                BaudRate     = config.BaudRate,
                DataBits     = config.DataBits,
                Parity       = config.Parity,
                StopBits     = config.StopBits,
                ReadTimeout  = config.TimeoutMs,
                WriteTimeout = config.TimeoutMs,
                Handshake    = Handshake.None,
                DtrEnable    = true,
                RtsEnable    = true
            };
            _port.Open();
            return true;
        }
        catch (Exception)
        {
            _port?.Dispose();
            _port = null;
            return false;
        }
    }

    public void Close()
    {
        if (_port is { IsOpen: true })
        {
            _port.Close();
        }
        _port?.Dispose();
        _port = null;
    }

    public int Write(ReadOnlySpan<byte> data)
    {
        if (_port is not { IsOpen: true })
            return -1;

        try
        {
            byte[] array = data.ToArray();
            _port.Write(array, 0, array.Length);
            return array.Length;
        }
        catch (TimeoutException)
        {
            return -1;
        }
        catch (Exception)
        {
            return -1;
        }
    }

    public int Read(Span<byte> buffer)
    {
        if (_port is not { IsOpen: true })
            return -1;

        try
        {
            byte[] array = new byte[buffer.Length];
            int totalRead = 0;

            while (totalRead < array.Length)
            {
                int bytesRead = _port.Read(array, totalRead, array.Length - totalRead);
                if (bytesRead == 0)
                    break;
                totalRead += bytesRead;
            }

            array.AsSpan(0, totalRead).CopyTo(buffer);
            return totalRead;
        }
        catch (TimeoutException)
        {
            return 0;
        }
        catch (Exception)
        {
            return -1;
        }
    }

    public void SetTimeout(int timeoutMs)
    {
        if (_port != null)
        {
            _port.ReadTimeout  = timeoutMs;
            _port.WriteTimeout = timeoutMs;
        }
    }

    public void FlushInput()
    {
        if (_port is { IsOpen: true })
        {
            _port.DiscardInBuffer();
        }
    }

    public void Dispose()
    {
        Close();
    }
}
