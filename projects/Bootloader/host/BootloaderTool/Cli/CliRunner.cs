// ----------------------------------------------------------------------------
//  CliRunner.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  CLI front door. Parses argv, prints help, and dispatches to the matching
//  verb. Holds the command registry and returns the process exit code.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Cli.Commands;

namespace BootloaderTool.Cli;

/// <summary>
/// Routes a command line to a registered <see cref="ICliCommand"/>. Parsing is
/// delegated to <see cref="CliParser"/>; this type owns the registry, help
/// output, and exit-code selection for parse-level outcomes.
/// </summary>
public static class CliRunner
{
    /// <summary>All registered verbs, in help-display order.</summary>
    public static readonly IReadOnlyList<ICliCommand> Commands = new ICliCommand[]
    {
        new ListCommand(),
        new InfoCommand(),
        new FactoryResetCommand(),
        new UploadCommand(),
        new VerifyCommand(),
        new ReadCommand(),
        new GoCommand(),
        new StampCommand(),
    };

    /// <summary>Parses, dispatches, and returns the process exit code.</summary>
    public static async Task<int> RunAsync(string[] args, CliContext context)
    {
        ParseResult parsed = CliParser.Parse(args);

        ICliCommand? command = parsed.Verb is null
            ? null
            : Commands.FirstOrDefault(c => c.Name == parsed.Verb);

        if (parsed.ShowHelp)
        {
            if (command is null)
                PrintTopLevelHelp(context.Out);
            else
                PrintCommandHelp(context.Out, command);
            return CliResult.Success;
        }

        if (parsed.Error is not null)
        {
            context.Err.WriteLine($"error: {parsed.Error}");
            context.Err.WriteLine("Run 'BootloaderTool --help' for usage.");
            return CliResult.Usage;
        }

        if (command is null)
        {
            context.Err.WriteLine($"error: unknown command '{parsed.Verb}'");
            context.Err.WriteLine("Run 'BootloaderTool --help' for the list of commands.");
            return CliResult.Usage;
        }

        return await command.ExecuteAsync(parsed.Options, context);
    }

    private static void PrintTopLevelHelp(TextWriter o)
    {
        o.WriteLine("BootloaderTool — STM32 UART (AN3155) bootloader utility");
        o.WriteLine();
        o.WriteLine("Usage: BootloaderTool <command> [options]");
        o.WriteLine();
        o.WriteLine("Commands:");
        int width = Commands.Max(c => c.Name.Length);
        foreach (ICliCommand c in Commands)
            o.WriteLine($"  {c.Name.PadRight(width)}  {c.Summary}");
        o.WriteLine();
        o.WriteLine("Run 'BootloaderTool <command> --help' for command-specific options.");
        o.WriteLine("Global options: --baud <n>  --timeout <ms>  --retries <n>  --json");
    }

    private static void PrintCommandHelp(TextWriter o, ICliCommand command)
    {
        o.WriteLine(command.Summary);
        o.WriteLine();
        o.WriteLine($"Usage: BootloaderTool {command.Usage}");
    }
}
