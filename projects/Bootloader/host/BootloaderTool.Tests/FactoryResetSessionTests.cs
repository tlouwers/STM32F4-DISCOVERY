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
//  mismatch retry exhaustion, and mid-write disconnect with reconnect+restart.
//  A no-op delay seam keeps the reconnect loop instantaneous (no real waits).
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
        // Get: ACK, N, version, command bytes, ACK
        _serial.EnqueueResponse(Ack);
        _serial.EnqueueResponse(3);
        _serial.EnqueueResponse(0x31);
        _serial.EnqueueResponse(new byte[] { 0x00, 0x31, 0x44 });
        _serial.EnqueueResponse(Ack);

        // Get ID: ACK, N=1, PID hi, PID lo, ACK  => 0x0413
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

    // ── Mid-write disconnect + reconnect ──────────────────────────────────────

    [Fact]
    public async Task RunAsync_DisconnectDuringWrite_ReconnectsAndCompletes()
    {
        // Initial script: connect + identify + erase only. Writing is then
        // interrupted by a simulated cable pull at the Writing transition.
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();

        var states = new List<FactoryResetState>();
        bool disconnected = false;
        bool reconnected  = false;

        var session = NewSession();
        session.StateChanged += s =>
        {
            states.Add(s);

            if (s == FactoryResetState.Writing && !disconnected)
            {
                // Pull the cable just as writing begins.
                disconnected = true;
                _serial.SimulateDisconnect();
            }
            else if (s == FactoryResetState.Connecting && disconnected && !reconnected)
            {
                // Cable plugged back in during the reconnect poll: restore the
                // link and script the full restart (sync → erase → write →
                // verify → go).
                reconnected = true;
                _serial.ClearDisconnect();
                EnqueueSyncAck();
                EnqueueErase();
                EnqueueWrite(2);
                EnqueueVerify(_image.Crc32);
                EnqueueGo();
            }
        };

        await session.RunAsync(_image);

        Assert.Equal(FactoryResetState.Done, session.State);

        // The state trace must show the restart: a second Connecting after the
        // first Writing, followed by Erasing and Writing again.
        int firstWriting = states.IndexOf(FactoryResetState.Writing);
        int reconnect    = states.IndexOf(FactoryResetState.Connecting, firstWriting);
        Assert.True(reconnect > firstWriting, "expected a reconnect after the interrupted write");
        Assert.Contains(FactoryResetState.Erasing, states.Skip(reconnect));
    }

    [Fact]
    public async Task RunAsync_DisconnectAndReconnectExhausted_FailsWithConnectionLost()
    {
        EnqueueSyncAck();
        EnqueueIdentify();
        EnqueueErase();

        bool disconnected = false;
        var session = NewSession(new FactoryResetOptions { MaxReconnectAttempts = 3 });
        session.StateChanged += s =>
        {
            if (s == FactoryResetState.Writing && !disconnected)
            {
                disconnected = true;
                _serial.SimulateDisconnect(); // never cleared -> reconnect fails
            }
        };

        await Assert.ThrowsAsync<ConnectionLostException>(() => session.RunAsync(_image));
        Assert.Equal(FactoryResetState.Failed, session.State);
    }
}
