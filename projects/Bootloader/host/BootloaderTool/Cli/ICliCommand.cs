// ----------------------------------------------------------------------------
//  ICliCommand.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Contract for a CLI verb. Each command names itself, supplies help text, and
//  executes against parsed options plus the ambient CliContext, returning a
//  process exit code.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Cli;

/// <summary>
/// A single CLI verb. Exit-code convention: 0 success, 1 runtime failure,
/// 2 usage error (missing/invalid options).
/// </summary>
public interface ICliCommand
{
    /// <summary>Verb name as typed on the command line (e.g. "factory-reset").</summary>
    string Name { get; }

    /// <summary>One-line summary shown in the top-level help listing.</summary>
    string Summary { get; }

    /// <summary>Usage line shown for this command's help.</summary>
    string Usage { get; }

    /// <summary>Runs the command and returns the process exit code.</summary>
    Task<int> ExecuteAsync(CliOptions options, CliContext context);
}
