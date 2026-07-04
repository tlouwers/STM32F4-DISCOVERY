// ----------------------------------------------------------------------------
//  CliRunnerTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for CliRunner dispatch and command exit codes. Commands are
//  driven against a scripted MockSerial injected through CliContext, so they
//  run without hardware.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Cli;
using BootloaderTool.Protocol.Crc;
using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;
using Xunit;

namespace BootloaderTool.Tests;

public class CliRunnerTests
{
    private const byte Ack = An3155Constants.Ack;

    private readonly StringWriter _out = new();
    private readonly StringWriter _err = new();

    /// <summary>Builds a context whose serial factory always returns <paramref name="serial"/>.</summary>
    private CliContext Context(ISerial? serial = null, params string[] ports) => new()
    {
        Out            = _out,
        Err            = _err,
        SerialFactory  = () => serial ?? new MockSerial(),
        PortNames      = () => ports,
    };

    // ── Dispatch / parse-level outcomes ──────────────────────────────────────

    [Fact]
    public async Task Run_NoArgs_PrintsHelp_ReturnsZero()
    {
        int code = await CliRunner.RunAsync(Array.Empty<string>(), Context());

        Assert.Equal(0, code);
        Assert.Contains("Commands:", _out.ToString());
    }

    [Fact]
    public async Task Run_UnknownCommand_ReturnsUsageError()
    {
        int code = await CliRunner.RunAsync(new[] { "frobnicate" }, Context());

        Assert.Equal(2, code);
        Assert.Contains("unknown command", _err.ToString());
    }

    [Fact]
    public async Task Run_ParseError_ReturnsUsageError()
    {
        int code = await CliRunner.RunAsync(new[] { "info", "--bogus" }, Context());

        Assert.Equal(2, code);
        Assert.Contains("unknown option", _err.ToString());
    }

    [Fact]
    public async Task Run_CommandHelp_PrintsUsage_ReturnsZero()
    {
        int code = await CliRunner.RunAsync(new[] { "factory-reset", "--help" }, Context());

        Assert.Equal(0, code);
        Assert.Contains("Usage: BootloaderTool factory-reset", _out.ToString());
    }

    // ── Usage errors from commands ───────────────────────────────────────────

    [Fact]
    public async Task Info_NoPort_ReturnsUsageError()
    {
        int code = await CliRunner.RunAsync(new[] { "info" }, Context());

        Assert.Equal(2, code);
        Assert.Contains("no serial port", _err.ToString());
    }

    [Fact]
    public async Task Read_MissingLength_ReturnsUsageError()
    {
        int code = await CliRunner.RunAsync(
            new[] { "read", "-p", "COM1", "--addr", "0x08000000", "-o", "x.bin" }, Context(new MockSerial()));

        Assert.Equal(2, code);
        Assert.Contains("no length", _err.ToString());
    }

    // ── Runtime outcomes via scripted MockSerial ─────────────────────────────

    [Fact]
    public async Task Info_NoSync_ReturnsFailure()
    {
        var serial = new MockSerial(); // nothing queued — sync times out
        int code = await CliRunner.RunAsync(new[] { "info", "-p", "COM1" }, Context(serial));

        Assert.Equal(1, code);
        Assert.Contains("did not respond", _err.ToString());
    }

    [Fact]
    public async Task Info_HappyPath_PrintsChipIdentity_ReturnsZero()
    {
        var serial = new MockSerial();
        serial.EnqueueResponse(Ack);                  // sync
        serial.EnqueueResponse(Ack);                  // Get: cmd ack
        serial.EnqueueResponse(3);                    // N = 3 bytes follow
        serial.EnqueueResponse(0x31);                 // protocol version
        serial.EnqueueResponse(0x00, 0x02, 0x44);     // supported commands
        serial.EnqueueResponse(Ack);                  // Get: trailing ack
        serial.EnqueueResponse(Ack);                  // GetId: cmd ack
        serial.EnqueueResponse(1);                    // N = PID length - 1
        serial.EnqueueResponse(0x04, 0x13);           // PID 0x0413
        serial.EnqueueResponse(Ack);                  // GetId: trailing ack

        int code = await CliRunner.RunAsync(new[] { "info", "-p", "COM1" }, Context(serial));

        Assert.Equal(0, code);
        string output = _out.ToString();
        Assert.Contains("0x0413", output);
        Assert.Contains("STM32F405/407/415/417", output);
        Assert.Contains("v3.1", output);
    }

    [Fact]
    public async Task Go_HappyPath_ReturnsZero()
    {
        var serial = new MockSerial();
        serial.EnqueueResponse(Ack); // sync
        serial.EnqueueResponse(Ack); // Go: cmd ack
        serial.EnqueueResponse(Ack); // Go: address ack

        int code = await CliRunner.RunAsync(
            new[] { "go", "-p", "COM1", "--addr", "0x08000000" }, Context(serial));

        Assert.Equal(0, code);
        Assert.Contains("Jumped to 0x08000000", _out.ToString());
    }

    [Fact]
    public async Task Verify_MatchingCrc_ReturnsZero()
    {
        // A small image whose CRC the (mock) device will echo back.
        byte[] imageBytes = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
        string path = Path.Combine(Path.GetTempPath(), $"verify_{Guid.NewGuid():N}.bin");
        File.WriteAllBytes(path, imageBytes);

        try
        {
            uint crc = new Crc32().Compute(imageBytes);
            var serial = new MockSerial();
            serial.EnqueueResponse(Ack);                                   // sync
            // Get: command list advertises 0xA1 => device-side GetChecksum path
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(4);
            serial.EnqueueResponse(0x31);
            serial.EnqueueResponse(new byte[] { 0x00, 0x31, 0x44, 0xA1 });
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(Ack);                                   // GetChecksum: cmd ack
            serial.EnqueueResponse(Ack);                                   // address ack
            serial.EnqueueResponse(Ack);                                   // word-count ack
            serial.EnqueueResponse((byte)(crc >> 24), (byte)(crc >> 16),
                                   (byte)(crc >> 8),  (byte)crc);          // CRC (big-endian)
            serial.EnqueueResponse(Ack);                                   // trailing ack

            int code = await CliRunner.RunAsync(
                new[] { "verify", "-p", "COM1", "-f", path }, Context(serial));

            Assert.Equal(0, code);
            Assert.Contains("CRC32 OK", _out.ToString());
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public async Task Verify_NoChecksumCommand_UsesReadBack_ReturnsZero()
    {
        byte[] imageBytes = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
        string path = Path.Combine(Path.GetTempPath(), $"verify_{Guid.NewGuid():N}.bin");
        File.WriteAllBytes(path, imageBytes);

        try
        {
            var serial = new MockSerial();
            serial.EnqueueResponse(Ack);                                   // sync
            // Get: command list WITHOUT 0xA1 (STM32F4) => read-back path
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(3);
            serial.EnqueueResponse(0x31);
            serial.EnqueueResponse(new byte[] { 0x00, 0x31, 0x44 });
            serial.EnqueueResponse(Ack);
            // ReadRegion -> one Read Memory chunk: cmd ack, address ack, count ack, data
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(Ack);
            serial.EnqueueResponse(imageBytes);

            int code = await CliRunner.RunAsync(
                new[] { "verify", "-p", "COM1", "-f", path }, Context(serial));

            Assert.Equal(0, code);
            Assert.Contains("CRC32 OK", _out.ToString());
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public async Task List_ProbesPorts_ReportsBootloader()
    {
        var serial = new MockSerial();
        serial.EnqueueResponse(Ack); // single port answers sync

        int code = await CliRunner.RunAsync(new[] { "list" }, Context(serial, "COM_TEST"));

        Assert.Equal(0, code);
        string output = _out.ToString();
        Assert.Contains("COM_TEST", output);
        Assert.Contains("bootloader", output);
    }

    // ── Stamp (offline, no port) ─────────────────────────────────────────────

    [Fact]
    public async Task Stamp_NoFile_ReturnsUsageError()
    {
        int code = await CliRunner.RunAsync(new[] { "stamp" }, Context());

        Assert.Equal(2, code);
        Assert.Contains("no firmware image", _err.ToString());
    }

    [Fact]
    public async Task Stamp_HeaderPresent_WritesStampedFile_ReturnsZero()
    {
        byte[] image = new byte[64];
        ImageHeader.Magic.CopyTo(image, 0);
        System.Text.Encoding.ASCII.GetBytes("F4DISCO1").CopyTo(image, 8);

        string path = Path.GetTempFileName();
        try
        {
            File.WriteAllBytes(path, image);

            int code = await CliRunner.RunAsync(new[] { "stamp", "-f", path }, Context());

            Assert.Equal(0, code);
            Assert.Contains("Stamped F4DISCO1", _out.ToString());

            ImageHeader? header = ImageHeader.FindIn(File.ReadAllBytes(path));
            Assert.NotNull(header);
            Assert.Equal(64u, header!.ImageSize);
            Assert.NotEqual(0u, header.HeaderCrc);
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public async Task Stamp_NoHeaderInImage_ReturnsFailure()
    {
        string path = Path.GetTempFileName();
        try
        {
            File.WriteAllBytes(path, new byte[64]);

            int code = await CliRunner.RunAsync(new[] { "stamp", "-f", path }, Context());

            Assert.Equal(1, code);
            Assert.Contains("no TLFWIMG1 header", _err.ToString());
        }
        finally
        {
            File.Delete(path);
        }
    }
}
