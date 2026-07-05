// ----------------------------------------------------------------------------
//  UploadCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "upload" verb — write a firmware image to a device already in bootloader
//  mode (erase -> write -> verify), without the chip guard and without booting.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Cli.Commands;

/// <summary>Writes and verifies firmware, leaving the device in bootloader mode.</summary>
public sealed class UploadCommand : ICliCommand
{
    public string Name    => "upload";
    public string Summary => "Write and verify firmware on a device already in bootloader mode.";
    public string Usage   => "upload -p <port> -f <image.bin> [--addr <hex>] [--force] " +
                             "[--baud <n>] [--timeout <ms>] [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
        => SessionCommand.RunAsync(options, context, runGo: false, guardChipId: false);
}
