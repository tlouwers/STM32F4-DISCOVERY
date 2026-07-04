// ----------------------------------------------------------------------------
//  An3155ClientTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for An3155Client, using MockSerial to simulate the ST
//  bootloader responses.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using Xunit;

namespace BootloaderTool.Tests;

public class An3155ClientTests : IDisposable
{
    private readonly MockSerial _serial;
    private readonly An3155Client _client;

    public An3155ClientTests()
    {
        _serial = new MockSerial();
        _serial.Open("COM_TEST", new Protocol.Serial.SerialConfig());
        _client = new An3155Client(_serial);
    }

    public void Dispose()
    {
        _serial.Dispose();
    }

    // ── Sync ───────────────────────────────────────────────────────────────

    [Fact]
    public void Sync_AckReceived_ReturnsTrue()
    {
        _serial.EnqueueResponse(An3155Constants.Ack);

        bool result = _client.Sync();

        Assert.True(result);
        // Verify 0x7F was sent
        Assert.Equal(An3155Constants.SyncByte, _serial.AllWrittenBytes[0]);
    }

    [Fact]
    public void Sync_Timeout_ReturnsFalse()
    {
        // No response queued — Read returns 0 (timeout)
        bool result = _client.Sync();

        Assert.False(result);
    }

    [Fact]
    public void Sync_NackReceived_ReturnsFalse()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        bool result = _client.Sync();

        Assert.False(result);
    }

    [Fact]
    public void Sync_Disconnect_ReturnsFalse()
    {
        _serial.SimulateDisconnect();

        bool result = _client.Sync();

        Assert.False(result);
    }

    // ── SyncWithRetries ──────────────────────────────────────────────────────

    [Fact]
    public void SyncWithRetries_AckReceived_ReturnsTrue()
    {
        _serial.EnqueueResponse(An3155Constants.Ack);

        bool result = _client.SyncWithRetries(attempts: 3, delayMs: 0);

        Assert.True(result);
        Assert.Equal(An3155Constants.SyncByte, _serial.AllWrittenBytes[0]);
    }

    [Fact]
    public void SyncWithRetries_NackReceived_ReturnsTrue()
    {
        // An already-initialised bootloader answers NACK to a bare 0x7F; that
        // still proves it is present, so the probe must treat it as synced.
        _serial.EnqueueResponse(An3155Constants.Nack);

        bool result = _client.SyncWithRetries(attempts: 3, delayMs: 0);

        Assert.True(result);
    }

    [Fact]
    public void SyncWithRetries_TimeoutThenAck_RetriesAndReturnsTrue()
    {
        _serial.EnqueueResponse();                     // attempt 1: empty → timeout
        _serial.EnqueueResponse(An3155Constants.Ack);  // attempt 2: ACK

        bool result = _client.SyncWithRetries(attempts: 3, delayMs: 0);

        Assert.True(result);
        // Two sync bytes sent: one per attempt up to the ACK.
        Assert.Equal(2, _serial.AllWrittenBytes.Count(b => b == An3155Constants.SyncByte));
    }

    [Fact]
    public void SyncWithRetries_AllTimeouts_ReturnsFalseAfterExhaustingAttempts()
    {
        // No response queued — every Read times out.
        bool result = _client.SyncWithRetries(attempts: 3, delayMs: 0);

        Assert.False(result);
        Assert.Equal(3, _serial.AllWrittenBytes.Count(b => b == An3155Constants.SyncByte));
    }

    // ── Get ────────────────────────────────────────────────────────────────

    [Fact]
    public void Get_HappyPath_ReturnsVersionAndCommands()
    {
        // Response: ACK, N=7, version=0x31, 7 command bytes, ACK
        byte[] commands = { 0x00, 0x01, 0x02, 0x11, 0x21, 0x31, 0x44 };
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(7);                    // N = 7 bytes follow
        _serial.EnqueueResponse(0x31);                 // protocol version
        _serial.EnqueueResponse(commands);             // supported commands
        _serial.EnqueueResponse(An3155Constants.Ack); // trailing ACK

        var result = _client.Get();

        Assert.Equal(0x31, result.ProtocolVersion);
        Assert.Equal(commands, result.SupportedCommands);
    }

    [Fact]
    public void Get_Nack_ThrowsNackException()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        Assert.Throws<NackException>(() => _client.Get());
    }

    [Fact]
    public void Get_Timeout_ThrowsTimeoutException()
    {
        // No response at all
        Assert.Throws<BootloaderTimeoutException>(() => _client.Get());
    }

    // ── Get Version ────────────────────────────────────────────────────────

    [Fact]
    public void GetVersion_HappyPath_ReturnsVersion()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(0x31);                 // version
        _serial.EnqueueResponse(0x00);                 // option byte 1
        _serial.EnqueueResponse(0x00);                 // option byte 2
        _serial.EnqueueResponse(An3155Constants.Ack); // trailing ACK

        byte version = _client.GetVersion();

        Assert.Equal(0x31, version);
    }

    // ── Get ID ─────────────────────────────────────────────────────────────

    [Fact]
    public void GetId_HappyPath_ReturnsChipId()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(0x01);                 // N = 1 (2 PID bytes - 1)
        _serial.EnqueueResponse(0x04);                 // PID high
        _serial.EnqueueResponse(0x13);                 // PID low
        _serial.EnqueueResponse(An3155Constants.Ack); // trailing ACK

        ushort chipId = _client.GetId();

        Assert.Equal(0x0413, chipId); // STM32F40x/41x
    }

    [Fact]
    public void GetId_Nack_ThrowsNackException()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        Assert.Throws<NackException>(() => _client.GetId());
    }

    // ── Read Memory ────────────────────────────────────────────────────────

    [Fact]
    public void ReadMemory_HappyPath_ReturnsData()
    {
        byte[] expected = { 0xDE, 0xAD, 0xBE, 0xEF };

        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // address ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // N ACK
        _serial.EnqueueResponse(expected);             // data

        byte[] data = _client.ReadMemory(0x08000000, 4);

        Assert.Equal(expected, data);
    }

    [Fact]
    public void ReadMemory_InvalidLength_Throws()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => _client.ReadMemory(0x08000000, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => _client.ReadMemory(0x08000000, 257));
    }

    [Fact]
    public void ReadRegion_MultiChunk_ReportsPerChunkProgress()
    {
        // 260 bytes = one full 256-byte chunk + one 4-byte remainder.
        for (int chunk = 0; chunk < 2; chunk++)
        {
            _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
            _serial.EnqueueResponse(An3155Constants.Ack); // address ACK
            _serial.EnqueueResponse(An3155Constants.Ack); // N ACK
            _serial.EnqueueResponse(new byte[chunk == 0 ? 256 : 4]); // data
        }

        var progress = new List<(int done, int total)>();
        byte[] data = _client.ReadRegion(0x08000000, 260, (done, total) => progress.Add((done, total)));

        Assert.Equal(260, data.Length);
        Assert.Equal(new[] { (256, 260), (260, 260) }, progress);
    }

    // ── Go ─────────────────────────────────────────────────────────────────

    [Fact]
    public void Go_HappyPath_SendsAddressAndAcks()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // address ACK

        _client.Go(0x08000000);

        // Verify command bytes sent: 0x21, 0xDE
        byte[] written = _serial.AllWrittenBytes;
        Assert.Equal(An3155Constants.CmdGo, written[0]);
        Assert.Equal((byte)(An3155Constants.CmdGo ^ 0xFF), written[1]);

        // Verify address bytes: 0x08, 0x00, 0x00, 0x00, checksum
        Assert.Equal(0x08, written[2]);
        Assert.Equal(0x00, written[3]);
        Assert.Equal(0x00, written[4]);
        Assert.Equal(0x00, written[5]);
        Assert.Equal(0x08, written[6]); // XOR checksum = 0x08
    }

    [Fact]
    public void Go_Nack_ThrowsNackException()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        Assert.Throws<NackException>(() => _client.Go(0x08000000));
    }

    // ── Write Memory ───────────────────────────────────────────────────────

    [Fact]
    public void WriteMemory_HappyPath_SendsDataWithChecksum()
    {
        byte[] payload = { 0x01, 0x02, 0x03, 0x04 };

        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // address ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // data ACK

        _client.WriteMemory(0x08000000, payload);

        // Verify the data frame was sent correctly
        // Command (2) + Address (5) + Data frame (1 + 4 + 1 = 6) = 13 bytes total
        byte[] written = _serial.AllWrittenBytes;
        Assert.Equal(13, written.Length);

        // Data frame starts at offset 7: N-1, D0..D3, checksum
        Assert.Equal(3, written[7]);    // N-1 = 3
        Assert.Equal(0x01, written[8]);
        Assert.Equal(0x02, written[9]);
        Assert.Equal(0x03, written[10]);
        Assert.Equal(0x04, written[11]);

        // Checksum: N-1 ^ D0 ^ D1 ^ D2 ^ D3 = 3 ^ 1 ^ 2 ^ 3 ^ 4 = 7 (0x07 ^  ... let me calculate)
        byte expectedChecksum = (byte)(3 ^ 0x01 ^ 0x02 ^ 0x03 ^ 0x04);
        Assert.Equal(expectedChecksum, written[12]);
    }

    [Fact]
    public void WriteMemory_InvalidLength_Throws()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            _client.WriteMemory(0x08000000, ReadOnlySpan<byte>.Empty));
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            _client.WriteMemory(0x08000000, new byte[257]));
    }

    [Fact]
    public void WriteMemory_Nack_ThrowsNackException()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        Assert.Throws<NackException>(() =>
            _client.WriteMemory(0x08000000, new byte[] { 0x01 }));
    }

    // ── Extended Erase ─────────────────────────────────────────────────────

    [Fact]
    public void ExtendedErase_HappyPath_SendsSectorsWithChecksum()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // erase ACK

        _client.ExtendedErase(new ushort[] { 0, 1, 2, 3, 4 });

        // Verify command bytes sent
        byte[] written = _serial.AllWrittenBytes;
        Assert.Equal(An3155Constants.CmdExtendedErase, written[0]);
        Assert.Equal((byte)(An3155Constants.CmdExtendedErase ^ 0xFF), written[1]);

        // Frame starts at offset 2: N-1 (2 bytes), sectors (2 bytes each), checksum (1 byte)
        // N-1 = 4 (5 sectors - 1)
        Assert.Equal(0x00, written[2]); // N-1 high
        Assert.Equal(0x04, written[3]); // N-1 low
    }

    [Fact]
    public void ExtendedErase_Nack_ThrowsNackException()
    {
        _serial.EnqueueResponse(An3155Constants.Nack);

        Assert.Throws<NackException>(() =>
            _client.ExtendedErase(new ushort[] { 0 }));
    }

    // ── Mass Erase ─────────────────────────────────────────────────────────

    [Fact]
    public void MassErase_HappyPath_SendsSpecialFrame()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // erase ACK

        _client.MassErase();

        byte[] written = _serial.AllWrittenBytes;
        // Command (2 bytes) + mass erase frame (0xFF, 0xFF, 0x00)
        Assert.Equal(0xFF, written[2]);
        Assert.Equal(0xFF, written[3]);
        Assert.Equal(0x00, written[4]);
    }

    // ── Get Checksum ───────────────────────────────────────────────────────

    [Fact]
    public void GetChecksum_HappyPath_ReturnsCrc()
    {
        uint expectedCrc = 0xA3B2C1D0;

        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // address ACK
        _serial.EnqueueResponse(An3155Constants.Ack); // count ACK
        _serial.EnqueueResponse(0xA3, 0xB2, 0xC1, 0xD0); // CRC result
        _serial.EnqueueResponse(An3155Constants.Ack); // trailing ACK

        uint crc = _client.GetChecksum(0x08000000, 1024);

        Assert.Equal(expectedCrc, crc);
    }

    // ── Connection loss ────────────────────────────────────────────────────

    [Fact]
    public void Get_ConnectionLost_ThrowsConnectionLostException()
    {
        _serial.EnqueueResponse(An3155Constants.Ack); // cmd ACK passes
        _serial.SimulateDisconnect();                  // then disconnect

        Assert.Throws<ConnectionLostException>(() => _client.Get());
    }

    [Fact]
    public void WriteMemory_ConnectionLost_ThrowsConnectionLostException()
    {
        _serial.SimulateDisconnect();

        Assert.Throws<ConnectionLostException>(() =>
            _client.WriteMemory(0x08000000, new byte[] { 0x01 }));
    }

    // ── Address framing ────────────────────────────────────────────────────

    [Fact]
    public void Go_AddressChecksum_IsCorrect()
    {
        _serial.EnqueueResponse(An3155Constants.Ack);
        _serial.EnqueueResponse(An3155Constants.Ack);

        _client.Go(0x1FFF0000);

        byte[] written = _serial.AllWrittenBytes;
        // Address at offset 2: 0x1F, 0xFF, 0x00, 0x00, checksum
        Assert.Equal(0x1F, written[2]);
        Assert.Equal(0xFF, written[3]);
        Assert.Equal(0x00, written[4]);
        Assert.Equal(0x00, written[5]);
        byte expectedChecksum = (byte)(0x1F ^ 0xFF ^ 0x00 ^ 0x00);
        Assert.Equal(expectedChecksum, written[6]);
    }
}
