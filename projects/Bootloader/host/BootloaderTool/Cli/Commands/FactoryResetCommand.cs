// ----------------------------------------------------------------------------
//  FactoryResetCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  "factory-reset" verb — full erase -> write -> verify -> go cycle driven by
//  FactoryResetSession, with chip-ID guard.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;

namespace BootloaderTool.Cli.Commands;

/// <summary>Runs a complete factory reset: erase, write, verify, then boot.</summary>
public sealed class FactoryResetCommand : ICliCommand
{
    public string Name    => "factory-reset";
    public string Summary => "Erase, write, verify a firmware image, then jump to it.";
    public string Usage   => "factory-reset -p <port> -f <image.bin> [--addr <hex>] [--no-go] " +
                             "[--baud <n>] [--timeout <ms>] [--json]";

    public Task<int> ExecuteAsync(CliOptions options, CliContext context)
        => SessionCommand.RunAsync(options, context, runGo: !options.NoGo, guardChipId: true);
}
