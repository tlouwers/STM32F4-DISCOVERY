// ----------------------------------------------------------------------------
//  An3155Client.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  AN3155 USART bootloader protocol client. Implements all commands needed
//  for factory reset: Sync, Get, GetVersion, GetID, ExtendedErase,
//  WriteMemory, GetChecksum, Go, ReadMemory.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Result of the Get command: protocol version and list of supported commands.
/// </summary>
public sealed class GetResult
{
    public byte ProtocolVersion { get; init; }
    public byte[] SupportedCommands { get; init; } = Array.Empty<byte>();
}

/// <summary>
/// AN3155 USART bootloader protocol client.
/// </summary>
public sealed class An3155Client
{
    private readonly ISerial _serial;

    public An3155Client(ISerial serial)
    {
        _serial = serial;
    }

    // -----------------------------------------------------------------------
    // Sync (baud rate detection)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Sends the 0x7F sync byte and waits for ACK. The ST bootloader uses
    /// this to auto-detect the baud rate.
    /// </summary>
    /// <returns>True if ACK received.</returns>
    public bool Sync()
    {
        _serial.FlushInput();

        ReadOnlySpan<byte> syncByte = stackalloc byte[] { An3155Constants.SyncByte };
        if (_serial.Write(syncByte) < 0)
            return false;

        return ReadAck() == An3155Constants.Ack;
    }

    /// <summary>
    /// Sends the 0x7F sync byte, retrying until the bootloader answers or the
    /// attempts run out. The ST bootloader uses the first 0x7F after entry for
    /// baud detection and replies ACK; a bootloader that has <b>already</b> been
    /// initialised replies NACK to a bare 0x7F, which still proves it is present
    /// and listening for commands. Both replies count as synced — only a
    /// timeout (no byte) is retried, which also covers a device that has not yet
    /// entered bootloader mode. Use this for one-shot probes (info/list) where a
    /// single Sync() is fragile against the autobaud race or an already-armed
    /// device; the factory-reset session keeps its own retry loop around Sync().
    /// </summary>
    /// <param name="attempts">Maximum number of sync attempts (clamped to >= 1).</param>
    /// <param name="delayMs">Delay between attempts, in milliseconds.</param>
    /// <returns>True if the bootloader responded with ACK or NACK.</returns>
    public bool SyncWithRetries(int attempts = 5, int delayMs = 300)
    {
        if (attempts < 1)
            attempts = 1;

        ReadOnlySpan<byte> syncByte = stackalloc byte[] { An3155Constants.SyncByte };

        for (int attempt = 0; attempt < attempts; attempt++)
        {
            _serial.FlushInput();

            if (_serial.Write(syncByte) >= 0)
            {
                byte response = ReadAck();
                if (response == An3155Constants.Ack || response == An3155Constants.Nack)
                    return true;
            }

            if (attempt + 1 < attempts)
                Thread.Sleep(delayMs);
        }

        return false;
    }

    // -----------------------------------------------------------------------
    // Get (0x00)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Sends the Get command. Returns protocol version and supported command list.
    /// </summary>
    public GetResult Get()
    {
        SendCommand(An3155Constants.CmdGet);
        ExpectAck(An3155Constants.CmdGet);

        // Read N (number of bytes to follow, not including this byte or final ACK)
        byte n = ReadByte();

        // First byte after N is protocol version
        byte version = ReadByte();

        // Remaining N bytes are the supported command codes
        byte[] commands = new byte[n];
        for (int i = 0; i < n; i++)
        {
            commands[i] = ReadByte();
        }

        ExpectAck(An3155Constants.CmdGet);

        return new GetResult
        {
            ProtocolVersion = version,
            SupportedCommands = commands
        };
    }

    // -----------------------------------------------------------------------
    // Get Version (0x01)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Sends the Get Version command. Returns the protocol version byte.
    /// </summary>
    public byte GetVersion()
    {
        SendCommand(An3155Constants.CmdGetVersion);
        ExpectAck(An3155Constants.CmdGetVersion);

        byte version = ReadByte();
        // Two option bytes follow (reserved, usually 0x00)
        ReadByte();
        ReadByte();

        ExpectAck(An3155Constants.CmdGetVersion);

        return version;
    }

    // -----------------------------------------------------------------------
    // Get ID (0x02)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Sends the Get ID command. Returns the chip ID (e.g. 0x0413 for STM32F40x).
    /// </summary>
    public ushort GetId()
    {
        SendCommand(An3155Constants.CmdGetId);
        ExpectAck(An3155Constants.CmdGetId);

        // N = number of bytes of PID - 1
        byte n = ReadByte();

        // Read PID bytes (big-endian)
        byte pidHigh = ReadByte();
        byte pidLow = (n >= 1) ? ReadByte() : (byte)0;

        ExpectAck(An3155Constants.CmdGetId);

        return (ushort)((pidHigh << 8) | pidLow);
    }

    // -----------------------------------------------------------------------
    // Read Memory (0x11)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Reads up to 256 bytes from the given address.
    /// </summary>
    /// <param name="address">Start address (must be aligned per device rules).</param>
    /// <param name="length">Number of bytes to read (1..256).</param>
    /// <returns>Data read from device.</returns>
    public byte[] ReadMemory(uint address, int length)
    {
        if (length < 1 || length > An3155Constants.MaxReadBytes)
            throw new ArgumentOutOfRangeException(nameof(length), "Must be 1..256");

        SendCommand(An3155Constants.CmdReadMemory);
        ExpectAck(An3155Constants.CmdReadMemory);

        SendAddress(address);
        ExpectAck(An3155Constants.CmdReadMemory);

        // Send N-1 and checksum
        byte nMinus1 = (byte)(length - 1);
        byte checksum = (byte)(nMinus1 ^ 0xFF);
        Span<byte> nFrame = stackalloc byte[] { nMinus1, checksum };
        WriteBytes(nFrame);

        ExpectAck(An3155Constants.CmdReadMemory);

        // Read the data
        byte[] data = new byte[length];
        ReadExact(data);
        return data;
    }

    /// <summary>
    /// Reads a contiguous region back in up to 256-byte chunks (Read Memory,
    /// 0x11). Use to verify a write on devices without the Get-Checksum command
    /// (0xA1) — e.g. STM32F4 — by CRC-comparing the returned bytes to the image.
    /// </summary>
    /// <param name="address">Start address.</param>
    /// <param name="length">Total number of bytes to read (>= 1).</param>
    /// <param name="progress">
    /// Optional per-chunk callback: (bytes read so far, total bytes). A full
    /// read-back at 115200 baud takes seconds per 100 KB, so callers surface
    /// this to the user rather than blocking silently.
    /// </param>
    /// <returns>The bytes read; the array length equals <paramref name="length"/>.</returns>
    public byte[] ReadRegion(uint address, int length, Action<int, int>? progress = null)
    {
        if (length < 1)
            throw new ArgumentOutOfRangeException(nameof(length), "Must be >= 1");

        byte[] result = new byte[length];
        int offset = 0;
        while (offset < length)
        {
            int chunk = Math.Min(An3155Constants.MaxReadBytes, length - offset);
            byte[] part = ReadMemory(address + (uint)offset, chunk);
            Array.Copy(part, 0, result, offset, chunk);
            offset += chunk;
            progress?.Invoke(offset, length);
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // Go (0x21)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Sends the Go command to jump to the given address.
    /// The bootloader sets the main stack pointer and jumps.
    /// </summary>
    /// <param name="address">Application start address (e.g. 0x08000000).</param>
    public void Go(uint address)
    {
        SendCommand(An3155Constants.CmdGo);
        ExpectAck(An3155Constants.CmdGo);

        SendAddress(address);
        ExpectAck(An3155Constants.CmdGo);
    }

    // -----------------------------------------------------------------------
    // Write Memory (0x31)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Writes up to 256 bytes to the given address.
    /// </summary>
    /// <param name="address">Start address (word-aligned for flash).</param>
    /// <param name="data">Data to write (1..256 bytes).</param>
    public void WriteMemory(uint address, ReadOnlySpan<byte> data)
    {
        if (data.Length < 1 || data.Length > An3155Constants.MaxWriteBytes)
            throw new ArgumentOutOfRangeException(nameof(data), "Must be 1..256 bytes");

        SendCommand(An3155Constants.CmdWriteMemory);
        ExpectAck(An3155Constants.CmdWriteMemory);

        SendAddress(address);
        ExpectAck(An3155Constants.CmdWriteMemory);

        // Send data: N-1, D0..DN-1, checksum
        byte nMinus1 = (byte)(data.Length - 1);
        byte checksum = nMinus1;
        for (int i = 0; i < data.Length; i++)
        {
            checksum ^= data[i];
        }

        byte[] frame = new byte[1 + data.Length + 1];
        frame[0] = nMinus1;
        data.CopyTo(frame.AsSpan(1));
        frame[^1] = checksum;

        WriteBytes(frame);
        ExpectAck(An3155Constants.CmdWriteMemory);
    }

    // -----------------------------------------------------------------------
    // Extended Erase (0x44)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Erases the specified flash sectors using the Extended Erase command.
    /// </summary>
    /// <param name="sectors">Sector numbers to erase (2-byte each).</param>
    /// <param name="eraseTimeoutMs">Timeout for the erase ACK (default 60 s).</param>
    public void ExtendedErase(ushort[] sectors, int eraseTimeoutMs = 60000)
    {
        SendCommand(An3155Constants.CmdExtendedErase);
        ExpectAck(An3155Constants.CmdExtendedErase);

        // Frame: N-1 (2 bytes, big-endian), then N sector numbers (2 bytes each), then checksum
        int n = sectors.Length;
        ushort nMinus1 = (ushort)(n - 1);

        byte[] frame = new byte[2 + n * 2 + 1];
        frame[0] = (byte)(nMinus1 >> 8);
        frame[1] = (byte)(nMinus1 & 0xFF);

        byte checksum = (byte)(frame[0] ^ frame[1]);

        for (int i = 0; i < n; i++)
        {
            byte hi = (byte)(sectors[i] >> 8);
            byte lo = (byte)(sectors[i] & 0xFF);
            frame[2 + i * 2]     = hi;
            frame[2 + i * 2 + 1] = lo;
            checksum ^= hi;
            checksum ^= lo;
        }
        frame[^1] = checksum;

        WriteBytes(frame);

        // Erase can take a long time — temporarily increase timeout
        int savedTimeout = _serial is SerialPortAdapter ? 2000 : 0;
        _serial.SetTimeout(eraseTimeoutMs);
        try
        {
            ExpectAck(An3155Constants.CmdExtendedErase);
        }
        finally
        {
            _serial.SetTimeout(savedTimeout > 0 ? savedTimeout : 2000);
        }
    }

    /// <summary>
    /// Performs a mass erase (erase all sectors).
    /// </summary>
    /// <param name="eraseTimeoutMs">Timeout for the erase ACK (default 60 s).</param>
    public void MassErase(int eraseTimeoutMs = 60000)
    {
        SendCommand(An3155Constants.CmdExtendedErase);
        ExpectAck(An3155Constants.CmdExtendedErase);

        // Special mass erase: 0xFFFF + checksum 0x00
        byte[] frame = { 0xFF, 0xFF, 0x00 };
        WriteBytes(frame);

        _serial.SetTimeout(eraseTimeoutMs);
        try
        {
            ExpectAck(An3155Constants.CmdExtendedErase);
        }
        finally
        {
            _serial.SetTimeout(2000);
        }
    }

    // -----------------------------------------------------------------------
    // Get Checksum (0xA1)
    // -----------------------------------------------------------------------

    /// <summary>
    /// Requests the device to compute CRC over a memory region.
    /// </summary>
    /// <param name="address">Start address (must be word-aligned).</param>
    /// <param name="wordCount">Number of 32-bit words to checksum.</param>
    /// <returns>CRC-32 computed by the STM32 hardware CRC peripheral.</returns>
    public uint GetChecksum(uint address, uint wordCount)
    {
        SendCommand(An3155Constants.CmdGetChecksum);
        ExpectAck(An3155Constants.CmdGetChecksum);

        SendAddress(address);
        ExpectAck(An3155Constants.CmdGetChecksum);

        // Send word count (4 bytes big-endian) + checksum
        byte[] countFrame = new byte[5];
        countFrame[0] = (byte)(wordCount >> 24);
        countFrame[1] = (byte)(wordCount >> 16);
        countFrame[2] = (byte)(wordCount >> 8);
        countFrame[3] = (byte)(wordCount);
        countFrame[4] = (byte)(countFrame[0] ^ countFrame[1] ^ countFrame[2] ^ countFrame[3]);
        WriteBytes(countFrame);

        ExpectAck(An3155Constants.CmdGetChecksum);

        // Read CRC result (4 bytes big-endian)
        Span<byte> crcBytes = stackalloc byte[4];
        ReadExact(crcBytes);

        // Read trailing ACK
        ExpectAck(An3155Constants.CmdGetChecksum);

        return ((uint)crcBytes[0] << 24) |
               ((uint)crcBytes[1] << 16) |
               ((uint)crcBytes[2] << 8)  |
               ((uint)crcBytes[3]);
    }

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// <summary>Sends a command byte and its complement.</summary>
    private void SendCommand(byte cmd)
    {
        Span<byte> frame = stackalloc byte[] { cmd, (byte)(cmd ^ 0xFF) };
        WriteBytes(frame);
    }

    /// <summary>Sends a 4-byte big-endian address with XOR checksum.</summary>
    private void SendAddress(uint address)
    {
        byte a3 = (byte)(address >> 24);
        byte a2 = (byte)(address >> 16);
        byte a1 = (byte)(address >> 8);
        byte a0 = (byte)(address);
        byte checksum = (byte)(a3 ^ a2 ^ a1 ^ a0);

        Span<byte> frame = stackalloc byte[] { a3, a2, a1, a0, checksum };
        WriteBytes(frame);
    }

    /// <summary>Reads a single ACK/NACK byte.</summary>
    private byte ReadAck()
    {
        Span<byte> buf = stackalloc byte[1];
        int n = _serial.Read(buf);
        if (n < 0)
            throw new ConnectionLostException("Serial read failed during ACK");
        if (n == 0)
            return 0; // timeout
        return buf[0];
    }

    /// <summary>Reads a single ACK byte and throws on NACK or timeout.</summary>
    private void ExpectAck(byte command)
    {
        byte response = ReadAck();
        if (response == An3155Constants.Ack)
            return;
        if (response == An3155Constants.Nack)
            throw new NackException(command);
        if (response == 0)
            throw new BootloaderTimeoutException($"Timeout waiting for ACK to command 0x{command:X2}");

        throw new BootloaderTimeoutException(
            $"Unexpected response 0x{response:X2} to command 0x{command:X2}");
    }

    /// <summary>Reads a single byte (throws on timeout or error).</summary>
    private byte ReadByte()
    {
        Span<byte> buf = stackalloc byte[1];
        int n = _serial.Read(buf);
        if (n < 0)
            throw new ConnectionLostException("Serial read failed");
        if (n == 0)
            throw new BootloaderTimeoutException("Timeout reading byte");
        return buf[0];
    }

    /// <summary>Reads exactly buffer.Length bytes (throws on short read).</summary>
    private void ReadExact(Span<byte> buffer)
    {
        int totalRead = 0;
        while (totalRead < buffer.Length)
        {
            int n = _serial.Read(buffer.Slice(totalRead));
            if (n < 0)
                throw new ConnectionLostException("Serial read failed");
            if (n == 0)
                throw new BootloaderTimeoutException("Timeout reading data");
            totalRead += n;
        }
    }

    /// <summary>Writes all bytes (throws on error).</summary>
    private void WriteBytes(ReadOnlySpan<byte> data)
    {
        int n = _serial.Write(data);
        if (n < 0)
            throw new ConnectionLostException("Serial write failed");
    }
}
