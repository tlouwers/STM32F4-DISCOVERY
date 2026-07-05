// ----------------------------------------------------------------------------
//  StampCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "stamp" verb — post-build step that fills in the imageSize and headerCrc
//  fields of an image's TLFWIMG1 metadata header (the two fields firmware
//  cannot compute at compile time). Offline: no device or port involved.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;

namespace BootloaderTool.Cli.Commands;

/// <summary>Fills in the stamped fields of an image's TLFWIMG1 metadata header.</summary>
public sealed class StampCommand : ICliCommand
{
    public string Name    => "stamp";
    public string Summary => "Fill in imageSize + headerCrc in an image's TLFWIMG1 header (post-build).";
    public string Usage   => "stamp -f <image.bin> [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
    {
        if (string.IsNullOrWhiteSpace(options.File))
            return Task.FromResult(CliResult.UsageError(context, "no firmware image specified (use -f <image.bin>)"));

        try
        {
            byte[] data = File.ReadAllBytes(options.File);
            ImageHeader header = ImageHeader.Stamp(data);
            File.WriteAllBytes(options.File, data);

            if (options.Json)
            {
                context.Out.WriteLine(System.Text.Json.JsonSerializer.Serialize(new
                {
                    product   = header.Product,
                    version   = header.VersionText,
                    offset    = $"0x{header.Offset:X}",
                    imageSize = header.ImageSize,
                    headerCrc = $"0x{header.HeaderCrc:X8}",
                }));
            }
            else
            {
                context.Out.WriteLine(
                    $"Stamped {header.Product} {header.VersionText} (header at 0x{header.Offset:X}): " +
                    $"imageSize {header.ImageSize:N0} bytes, headerCrc 0x{header.HeaderCrc:X8}.");
            }

            return Task.FromResult(CliResult.Success);
        }
        catch (InvalidOperationException ex)
        {
            return Task.FromResult(CliResult.Fail(context, ex.Message));
        }
        catch (Exception ex)
        {
            return Task.FromResult(CliResult.Fail(context, $"cannot stamp '{options.File}': {ex.Message}"));
        }
    }
}
