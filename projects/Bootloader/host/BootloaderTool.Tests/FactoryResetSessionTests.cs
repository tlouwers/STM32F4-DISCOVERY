// ----------------------------------------------------------------------------
//  FactoryResetSessionTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit end-to-end tests for FactoryResetSession over MockSerial: happy path,
//  state-machine progression, chip-ID guard, NACK abort, sync failure, CRC
//  mismatch retry exhaustion, and mid-write disconnect (fail fast, no reconnect).
//  A no-op delay seam keeps the initial sync poll instantaneous (no real waits).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;
using Xunit;

namespace BootloaderTool.Tests;

public class FactoryResetSessionTests : IDisposable
{
    private const byte Ack  = An3155Constants.Ack;
    private const byte Nack = An3155Constants.Nack;

    private readonly MockSerial _serial;

    // 260-byte image => two write chunks (256 + 4) within sector 0.
    private readonly FirmwareImage _image;

    public FactoryResetSessionTests()
    {
        _serial = new MockSerial();
        _serial.Open("COM_TEST", new SerialConfig());

        byte[] data = Enumerable.Range(0, 260).Select(i => (byte)i).ToArray();
        _image = new FirmwareImage(data);
    }

    public void Dispose() => _serial.Dispose();

    // No real time passes during (re)connect polling.
    private static Task NoDelay(int ms, CancellationToken ct) => Task.CompletedTask;

    private FactoryResetSession NewSession(FactoryResetOptions? options = null)
        => new FactoryResetSession(_serial, options, NoDelay);

    // ── Response-scripting helpers ───────────────────────────────────────────

    private void EnqueueSyncAck() => _serial.EnqueueResponse(Ack);

    private void EnqueueIdentify()
    {
        // Get: ACK, N, version, command bytes, ACK. Includes 0xA1 (Get-Checksum)
        // so the session verifies via the device-side CRC path.
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(4);
        _serial.EnqueueResponse(0x31);
        _serial.EnqueueResponse(new byte[] { 0x00, 0x31, 0x44, 0xA1 });
        _serial.EnqueueResponse(Ack);

        // Get ID: ACK, N=1, PID hi, PID lo, ACK  => 0x0413
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(0x01);
        _serial.EnqueueResponse(0x04);
        _serial.EnqueueResponse(0x13);
        _serial.EnqueueResponse(Ack);
    }

    private void EnqueueIdentifyNoChecksum()
    {
        // Same as EnqueueIdentify but the command list omits 0xA1, mirroring the
        // STM32F4 ROM bootloader, so the session must verify by read-back.
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(3);
        _serial.EnqueueResponse(0x31);
        _serial.EnqueueResponse(new byte[] { 0x00, 0x31, 0x44 });
        _serial.EnqueueResponse(Ack);

        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(0x01);
        _serial.EnqueueResponse(0x04);
        _serial.EnqueueResponse(0x13);
        _serial.EnqueueResponse(Ack);
    }

    private void EnqueueErase()
    {
        _serial.EnqueueResponse(Ack); // command ACK
        _serial.EnqueueResponse(Ack); // erase-complete ACK
    }

    private void EnqueueWrite(int chunks)
    {
        for (int i = 0; i < chunks; i++)
        {
            _serial.EnqueueResponse(Ack); // command ACK
            _serial.EnqueueResponse(Ack); // address ACK
            _serial.EnqueueResponse(Ack); // data ACK
        }
    }

    private void EnqueueVerify(uint crc)
    {
        _serial.EnqueueResponse(Ack); // command ACK
        _serial.EnqueueResponse(Ack); // address ACK
        _serial.EnqueueResponse(Ack); // count ACK
        _serial.EnqueueResponse((byte)(crc >> 24), (byte)(crc >> 16), (byte)(crc >> 8), (byte)crc);
        _serial.EnqueueResponse(Ack); // trailing ACK
    }

    private void EnqueueVerifyReadBack(byte[] contents)
    {
        // Read Memory (0x11) per 256-byte chunk: command ACK, address ACK,
        // count ACK, then the chunk data bytes (no trailing ACK).
        int offset = 0;
        while (offset < contents.Length)
        {
            int chunk = Math.Min(An3155Constants.MaxReadBytes, contents.Length - offset);
            _serial.EnqueueResponse(Ack);
            _serial.EnqueueResponse(Ack);
            _serial.EnqueueResponse(Ack);
            _serial.EnqueueResponse(contents.Skip(offset).Take(chunk).ToArray());
            offset += chunk;
        }
    }

    private void EnqueueGo()
    {
        _serial.EnqueueResponse(Ack); // command ACK
        _serial.EnqueueResponse(Ack); // address ACK
    }

    // ── Happy path ───────────────────────────────────────────────────────────

    [Fact]
    public async Task RunAsync_HappyPath_CompletesAtDone()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32);
        EnqueueGo();

        var session = NewSession();
        await session.RunAsync(_image);

        Assert.Equal(FactoryResetState.Done, session.State);
    }

    [Fact]
    public async Task RunAsync_UnalignedImageOnChecksumDevice_VerifiesAgainstWordAlignedCrc()
    {
        // The device CRCs WordCount whole words, so for a non-word-aligned
        // image it returns the 0xFF-padded (word-aligned) CRC — the session
        // must accept that, not demand the byte-exact image CRC.
        byte[] data = Enumerable.Range(0, 258).Select(i => (byte)i).ToArray();
        var image = new FirmwareImage(data);

        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(image.WordAlignedCrc32);
        EnqueueGo();

        var session = NewSession();
        await session.RunAsync(image);

        Assert.Equal(FactoryResetState.Done, session.State);
    }

    [Fact]
    public async Task RunAsync_HappyPath_VisitsStatesInOrder()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32);
        EnqueueGo();

        var states = new List<FactoryResetState>();
        var session = NewSession();
        session.StateChanged += s => states.Add(s);

        await session.RunAsync(_image);

        Assert.Equal(new[]
        {
            FactoryResetState.Connecting,
            FactoryResetState.Erasing,
            FactoryResetState.Writing,
            FactoryResetState.Verifying,
            FactoryResetState.Booting,
            FactoryResetState.Done,
        }, states);
    }

    [Fact]
    public async Task RunAsync_HappyPath_ReportsWriteProgressToCompletion()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32);
        EnqueueGo();

        var writeProgress = new List<(uint current, uint total)>();
        var session = NewSession();
        session.Progress += (stage, current, total) =>
        {
            if (stage == "Writing")
                writeProgress.Add((current, total));
        };

        await session.RunAsync(_image);

        Assert.Equal((256u, 260u), writeProgress[0]);
        Assert.Equal((260u, 260u), writeProgress[^1]);
    }

    [Fact]
    public async Task RunAsync_RunGoDisabled_StopsAtBootingWithoutGo()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32);
        // No Go response enqueued.

        var session = NewSession(new FactoryResetOptions { RunGo = false });
        await session.RunAsync(_image);

        Assert.Equal(FactoryResetState.Done, session.State);
    }

    // ── Read-back verify (device without Get-Checksum, e.g. STM32F4) ──────────

    [Fact]
    public async Task RunAsync_NoChecksumCommand_VerifiesByReadBack_CompletesAtDone()
    {
        EnqueueSyncAck();
        EnqueueIdentifyNoChecksum();
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerifyReadBack(_image.Data);  // exact bytes => read-back CRC matches
        EnqueueGo();

        var session = NewSession();
        await session.RunAsync(_image);

        Assert.Equal(FactoryResetState.Done, session.State);
    }

    [Fact]
    public async Task RunAsync_ReadBackMismatch_RetriesAndFailsAfterMaxAttempts()
    {
        EnqueueSyncAck();
        EnqueueIdentifyNoChecksum();

        // Two rounds, each reading back a corrupted first byte => CRC mismatch.
        for (int round = 0; round < 2; round++)
        {
            EnqueueErase();
            EnqueueWrite(2);
            byte[] corrupt = (byte[])_image.Data.Clone();
            corrupt[0] ^= 0xFF;
            EnqueueVerifyReadBack(corrupt);
        }

        var session = NewSession(new FactoryResetOptions { MaxWriteAttempts = 2 });

        await Assert.ThrowsAsync<ChecksumMismatchException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }

    // ── Identification guards ─────────────────────────────────────────────────

    [Fact]
    public async Task RunAsync_WrongChipId_FailsWithoutErasing()
    {
        EnqueueSyncAck();
        // Get
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(1);
        _serial.EnqueueResponse(0x31);
        _serial.EnqueueResponse(new byte[] { 0x44 });
        _serial.EnqueueResponse(Ack);
        // Get ID => 0x0400 (wrong)
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(0x01);
        _serial.EnqueueResponse(0x04);
        _serial.EnqueueResponse(0x00);
        _serial.EnqueueResponse(Ack);

        var session = NewSession();

        await Assert.ThrowsAsync<InvalidOperationException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }

    // ── Failure modes ─────────────────────────────────────────────────────────

    [Fact]
    public async Task RunAsync_SyncNeverAcks_FailsWithTimeout()
    {
        // Nothing enqueued -> every Sync times out.
        var session = NewSession(new FactoryResetOptions { SyncAttempts = 3 });

        await Assert.ThrowsAsync<BootloaderTimeoutException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }

    [Fact]
    public async Task RunAsync_EraseNacked_FailsWithNack()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        _serial.EnqueueResponse(Ack);  // erase command ACK
        _serial.EnqueueResponse(Nack); // erase rejected

        var session = NewSession();

        await Assert.ThrowsAsync<NackException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }

    [Fact]
    public async Task RunAsync_PersistentCrcMismatch_FailsAfterMaxAttempts()
    {
        EnqueueSyncAck();
        EnqueueIdentify();

        // Two full erase+write+verify rounds, both returning a bad CRC.
        for (int round = 0; round < 2; round++)
        {
            EnqueueErase();
            EnqueueWrite(2);
            EnqueueVerify(_image.Crc32 ^ 0xFFFFFFFF); // deliberately wrong
        }

        var session = NewSession(new FactoryResetOptions { MaxWriteAttempts = 2 });

        await Assert.ThrowsAsync<ChecksumMismatchException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }

    [Fact]
    public async Task RunAsync_TransientCrcMismatchThenMatch_Recovers()
    {
        EnqueueSyncAck();
        EnqueueIdentify();

        // First round: bad CRC. Second round: good CRC, then Go.
        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32 ^ 0xFFFFFFFF);

        EnqueueErase();
        EnqueueWrite(2);
        EnqueueVerify(_image.Crc32);
        EnqueueGo();

        var session = NewSession(new FactoryResetOptions { MaxWriteAttempts = 3 });
        await session.RunAsync(_image);

        Assert.Equal(FactoryResetState.Done, session.State);
    }

    // ── Mid-write disconnect: fail fast, no auto-reconnect ────────────────────

    [Fact]
    public async Task RunAsync_DisconnectDuringWrite_FailsFastWithConnectionLost()
    {
        // The ST bootloader cannot resume a partial write, so a mid-write loss
        // must fail immediately (no reconnect poll) and leave the session Failed.
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();

        var states = new List<FactoryResetState>();
        bool disconnected = false;

        var session = NewSession();
        session.StateChanged += s =>
        {
            states.Add(s);
            if (s == FactoryResetState.Writing && !disconnected)
            {
                disconnected = true;
                _serial.SimulateDisconnect(); // cable pull, never cleared
            }
        };

        await Assert.ThrowsAsync<ConnectionLostException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);

        // No second Connecting after the first Writing — the session does not
        // attempt to reconnect.
        int firstWriting = states.IndexOf(FactoryResetState.Writing);
        int reconnect    = states.IndexOf(FactoryResetState.Connecting, firstWriting);
        Assert.True(reconnect < 0, "expected no reconnect attempt after the interrupted write");
    }
}
