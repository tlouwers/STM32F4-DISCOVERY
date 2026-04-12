// ----------------------------------------------------------------------------
//  MockSerial.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Scriptable mock serial port for unit testing. Queue canned responses
//  (ACK, NACK, arbitrary data, timeouts) to simulate the ST bootloader.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Tests;

/// <summary>
/// Scriptable mock serial port. Enqueue responses that will be returned by
/// successive Read() calls, and inspect what was written.
/// </summary>
public sealed class MockSerial : ISerial
{
    private readonly Queue<byte[]> _responses = new();
    private readonly List<byte[]> _written = new();
    private byte[] _currentResponse = Array.Empty<byte>();
    private int _currentOffset;
    private bool _isOpen;
    private bool _simulateTimeout;
    private bool _simulateDisconnect;
    private int _timeoutMs = 2000;

    public bool IsOpen => _isOpen;

    // ---- Scripting API (used by tests) ------------------------------------

    /// <summary>Enqueue a canned response that Read() will return.</summary>
    public void EnqueueResponse(params byte[] data) => _responses.Enqueue(data);

    /// <summary>Simulate a timeout on the next Read() call.</summary>
    public void SimulateTimeout() => _simulateTimeout = true;

    /// <summary>Simulate a disconnect — Read/Write return -1.</summary>
    public void SimulateDisconnect() => _simulateDisconnect = true;

    /// <summary>Stop simulating disconnect.</summary>
    public void ClearDisconnect() => _simulateDisconnect = false;

    /// <summary>All byte arrays written so far.</summary>
    public IReadOnlyList<byte[]> WrittenData => _written;

    /// <summary>All bytes written, concatenated.</summary>
    public byte[] AllWrittenBytes => _written.SelectMany(b => b).ToArray();

    // ---- ISerial ----------------------------------------------------------

    public bool Open(string portName, SerialConfig config)
    {
        _isOpen = true;
        _timeoutMs = config.TimeoutMs;
        return true;
    }

    public void Close()
    {
        _isOpen = false;
    }

    public int Write(ReadOnlySpan<byte> data)
    {
        if (!_isOpen || _simulateDisconnect)
            return -1;

        _written.Add(data.ToArray());
        return data.Length;
    }

    public int Read(Span<byte> buffer)
    {
        if (!_isOpen || _simulateDisconnect)
            return -1;

        if (_simulateTimeout)
        {
            _simulateTimeout = false;
            return 0;
        }

        // If the current response is exhausted, dequeue the next one
        if (_currentOffset >= _currentResponse.Length)
        {
            if (_responses.Count == 0)
                return 0; // No more data — acts like timeout

            _currentResponse = _responses.Dequeue();
            _currentOffset = 0;
        }

        int available = _currentResponse.Length - _currentOffset;
        int toCopy = Math.Min(available, buffer.Length);
        _currentResponse.AsSpan(_currentOffset, toCopy).CopyTo(buffer);
        _currentOffset += toCopy;
        return toCopy;
    }

    public void SetTimeout(int timeoutMs)
    {
        _timeoutMs = timeoutMs;
    }

    public void FlushInput()
    {
        // Only discard partially-consumed data (simulates hardware buffer flush).
        // Do not clear pre-scripted responses — those represent future device
        // behaviour, not bytes already sitting in the OS receive buffer.
        _currentResponse = Array.Empty<byte>();
        _currentOffset = 0;
    }

    public void Dispose()
    {
        Close();
    }
}
