// ----------------------------------------------------------------------------
//  FactoryResetSession.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  High-level factory reset orchestrator. Sequences the full AN3155 operation
//  (sync → get → get-id → erase → write → verify → go) as a state machine,
//  raising progress/log events and recovering from a mid-transfer connection
//  loss by reconnecting and restarting from the erase step.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Crc;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Orchestrates a full factory reset over the AN3155 USART protocol. Drives an
/// <see cref="An3155Client"/> through the connect → erase → write → verify →
/// boot sequence, exposes a <see cref="FactoryResetState"/> state machine, and
/// recovers from a mid-transfer connection loss by polling for the bootloader
/// and restarting from the erase step.
/// </summary>
public sealed class FactoryResetSession
{
    private readonly An3155Client _client;
    private readonly FactoryResetOptions _options;
    private readonly Func<int, CancellationToken, Task> _delayAsync;

    private uint _lastDeviceCrc;

    // Whether the device advertises the Get-Checksum command (0xA1). The STM32F4
    // ROM bootloader does not, so verification falls back to read-back compare.
    private bool _deviceHasChecksum;

    /// <summary>Current state of the session.</summary>
    public FactoryResetState State { get; private set; } = FactoryResetState.Idle;

    /// <summary>Raised on every state transition.</summary>
    public event Action<FactoryResetState>? StateChanged;

    /// <summary>Raised on progress updates: (stage, current, total).</summary>
    public event Action<string, uint, uint>? Progress;

    /// <summary>Raised for log messages: (level, message).</summary>
    public event Action<string, string>? Log;

    /// <summary>
    /// Creates a factory reset session over the given serial port.
    /// </summary>
    /// <param name="serial">Open serial port to the device in bootloader mode.</param>
    /// <param name="options">Tunable parameters; defaults used when null.</param>
    /// <param name="delayAsync">
    /// Delay seam used between (re)connect poll attempts. Defaults to
    /// <see cref="Task.Delay(int, CancellationToken)"/>; tests inject a no-op
    /// so they run without real time passing.
    /// </param>
    public FactoryResetSession(
        ISerial serial,
        FactoryResetOptions? options = null,
        Func<int, CancellationToken, Task>? delayAsync = null)
    {
        if (serial is null)
            throw new ArgumentNullException(nameof(serial));

        _options    = options ?? new FactoryResetOptions();
        _delayAsync = delayAsync ?? ((ms, ct) => Task.Delay(ms, ct));
        _client     = new An3155Client(serial);
    }

    /// <summary>
    /// Runs the complete factory reset sequence to completion. On success the
    /// state ends at <see cref="FactoryResetState.Done"/>; on an unrecoverable
    /// error the state is set to <see cref="FactoryResetState.Failed"/> and the
    /// originating exception is rethrown.
    /// </summary>
    /// <param name="image">Firmware image to flash.</param>
    /// <param name="cancellationToken">Cancels the operation.</param>
    public async Task RunAsync(FirmwareImage image, CancellationToken cancellationToken = default)
    {
        if (image is null)
            throw new ArgumentNullException(nameof(image));

        try
        {
            TransitionTo(FactoryResetState.Connecting);
            if (!await TryConnectAsync(_options.SyncAttempts, _options.SyncDelayMs, cancellationToken))
                throw new BootloaderTimeoutException("Bootloader did not respond to sync (0x7F).");

            Identify();

            ushort[] sectors = Stm32F4FlashLayout.SectorsForRange(image.StartAddress, image.Size);
            Log?.Invoke("info",
                $"Image: {image.Size} bytes, CRC32 0x{image.Crc32:X8}, sectors {DescribeSectors(sectors)}.");

            int writeAttempts = 0;
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();
                try
                {
                    Erase(sectors);
                    Write(image, cancellationToken);

                    if (Verify(image))
                        break; // success

                    writeAttempts++;
                    if (writeAttempts >= _options.MaxWriteAttempts)
                        throw new ChecksumMismatchException(image.Crc32, _lastDeviceCrc);

                    Log?.Invoke("warn",
                        $"CRC32 mismatch; retrying erase+write ({writeAttempts}/{_options.MaxWriteAttempts}).");
                }
                catch (ConnectionLostException ex)
                {
                    Log?.Invoke("warn", $"Connection lost during transfer: {ex.Message}");
                    if (!await ReconnectAsync(cancellationToken))
                        throw; // reconnect exhausted — surface the original loss
                    // Reconnected: loop restarts from erase. A reconnect does not
                    // count against the CRC-mismatch write-attempt budget.
                }
            }

            Boot(image);
            TransitionTo(FactoryResetState.Done);
        }
        catch
        {
            TransitionTo(FactoryResetState.Failed);
            throw;
        }
    }

    // -----------------------------------------------------------------------
    // Sequence steps
    // -----------------------------------------------------------------------

    /// <summary>Polls 0x7F until the bootloader ACKs or attempts are exhausted.</summary>
    private async Task<bool> TryConnectAsync(int attempts, int delayMs, CancellationToken ct)
    {
        for (int i = 0; i < attempts; i++)
        {
            ct.ThrowIfCancellationRequested();
            if (_client.Sync())
            {
                Log?.Invoke("info", "Bootloader connected.");
                return true;
            }
            if (i < attempts - 1)
                await _delayAsync(delayMs, ct);
        }
        return false;
    }

    /// <summary>Reads protocol version and chip ID; enforces the chip-ID guard.</summary>
    private void Identify()
    {
        GetResult info = _client.Get();
        Log?.Invoke("info",
            $"Bootloader protocol v{info.ProtocolVersion >> 4}.{info.ProtocolVersion & 0x0F}, " +
            $"{info.SupportedCommands.Length} commands.");

        ushort chipId = _client.GetId();
        Log?.Invoke("info", $"Chip ID: 0x{chipId:X4}.");

        _deviceHasChecksum = info.SupportedCommands.Contains(An3155Constants.CmdGetChecksum);
        if (!_deviceHasChecksum)
            Log?.Invoke("info", "Device has no Get-Checksum (0xA1) command; verifying by read-back.");

        if (_options.ExpectedChipId.HasValue && chipId != _options.ExpectedChipId.Value)
            throw new InvalidOperationException(
                $"Chip ID mismatch: expected 0x{_options.ExpectedChipId.Value:X4}, got 0x{chipId:X4}.");
    }

    /// <summary>Erases the sectors the image occupies.</summary>
    private void Erase(ushort[] sectors)
    {
        TransitionTo(FactoryResetState.Erasing);
        Progress?.Invoke("Erasing", 0, (uint)sectors.Length);
        _client.ExtendedErase(sectors, _options.EraseTimeoutMs);
        Progress?.Invoke("Erasing", (uint)sectors.Length, (uint)sectors.Length);
        Log?.Invoke("info", $"Erased {sectors.Length} sector(s).");
    }

    /// <summary>Writes the image in 256-byte chunks at increasing addresses.</summary>
    private void Write(FirmwareImage image, CancellationToken ct)
    {
        TransitionTo(FactoryResetState.Writing);

        int total   = image.Size;
        int written = 0;
        while (written < total)
        {
            ct.ThrowIfCancellationRequested();
            int chunk = Math.Min(An3155Constants.MaxWriteBytes, total - written);
            _client.WriteMemory(image.StartAddress + (uint)written, image.Data.AsSpan(written, chunk));
            written += chunk;
            Progress?.Invoke("Writing", (uint)written, (uint)total);
        }

        Log?.Invoke("info", $"Wrote {total} bytes.");
    }

    /// <summary>
    /// Verifies the written image. Uses the device-side Get-Checksum command
    /// when available; otherwise (e.g. STM32F4) reads the region back and
    /// compares its CRC32.
    /// </summary>
    /// <returns>True if the device contents match the image.</returns>
    private bool Verify(FirmwareImage image)
    {
        TransitionTo(FactoryResetState.Verifying);

        if (_deviceHasChecksum)
        {
            Progress?.Invoke("Verifying", 0, 1);
            _lastDeviceCrc = _client.GetChecksum(image.StartAddress, image.WordCount);
            bool ok = _lastDeviceCrc == image.Crc32;
            Progress?.Invoke("Verifying", 1, 1);
            Log?.Invoke(ok ? "info" : "warn",
                $"Device CRC32 0x{_lastDeviceCrc:X8}, expected 0x{image.Crc32:X8}.");
            return ok;
        }

        return VerifyByReadBack(image);
    }

    /// <summary>
    /// Reads the flashed region back in 256-byte chunks (Read Memory, 0x11) and
    /// compares its CRC32 with the image. Used on devices without Get-Checksum.
    /// </summary>
    /// <returns>True if the read-back CRC32 matches the image CRC32.</returns>
    private bool VerifyByReadBack(FirmwareImage image)
    {
        Progress?.Invoke("Verifying", 0, 1);

        byte[] readBack = _client.ReadRegion(image.StartAddress, image.Size);
        _lastDeviceCrc = new Crc32().Compute(readBack);
        bool ok = _lastDeviceCrc == image.Crc32;

        Progress?.Invoke("Verifying", 1, 1);
        Log?.Invoke(ok ? "info" : "warn",
            $"Read-back CRC32 0x{_lastDeviceCrc:X8}, expected 0x{image.Crc32:X8}.");
        return ok;
    }

    /// <summary>Optionally jumps to the application via the Go command.</summary>
    private void Boot(FirmwareImage image)
    {
        TransitionTo(FactoryResetState.Booting);
        if (_options.RunGo)
        {
            _client.Go(image.StartAddress);
            Log?.Invoke("info", $"Jumped to 0x{image.StartAddress:X8}.");
        }
    }

    /// <summary>Polls for the bootloader after a connection loss.</summary>
    /// <returns>True if the bootloader responded within the attempt budget.</returns>
    private async Task<bool> ReconnectAsync(CancellationToken ct)
    {
        TransitionTo(FactoryResetState.Connecting);
        Log?.Invoke("info", "Attempting to reconnect...");

        for (int i = 0; i < _options.MaxReconnectAttempts; i++)
        {
            ct.ThrowIfCancellationRequested();
            await _delayAsync(_options.ReconnectDelayMs, ct);
            if (_client.Sync())
            {
                Log?.Invoke("info", "Reconnected.");
                return true;
            }
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    private void TransitionTo(FactoryResetState state)
    {
        State = state;
        StateChanged?.Invoke(state);
    }

    private static string DescribeSectors(ushort[] sectors)
    {
        if (sectors.Length == 0)
            return "(none)";
        if (sectors.Length == 1)
            return sectors[0].ToString();
        return $"{sectors[0]}-{sectors[^1]}";
    }
}
