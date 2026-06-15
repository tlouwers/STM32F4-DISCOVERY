// ----------------------------------------------------------------------------
//  Program.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Entry point. With arguments it runs CLI verb mode and returns an exit code;
//  with none it launches the Avalonia desktop GUI.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using Avalonia;
using BootloaderTool.Cli;

namespace BootloaderTool;

/// <summary>Process entry point and Avalonia bootstrap.</summary>
internal sealed class Program
{
    /// <summary>
    /// Routes to CLI mode when arguments are supplied, otherwise starts the GUI.
    /// Marked STA for desktop dialogs/clipboard on Windows.
    /// </summary>
    [STAThread]
    public static int Main(string[] args)
    {
        if (args.Length > 0)
            return CliRunner.RunAsync(args, CliContext.Default()).GetAwaiter().GetResult();

        BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        return 0;
    }

    /// <summary>Avalonia app builder (also used by the design-time tooling).</summary>
    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .LogToTrace();
}
