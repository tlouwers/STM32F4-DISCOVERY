// ----------------------------------------------------------------------------
//  LogEntry.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  A single line in the GUI activity log: severity level plus message.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.ViewModels;

/// <summary>One activity-log line shown in the GUI.</summary>
public sealed class LogEntry
{
    /// <summary>Severity level (e.g. "info", "warn", "error").</summary>
    public string Level { get; }

    /// <summary>Human-readable message text.</summary>
    public string Message { get; }

    /// <summary>Creates a log entry.</summary>
    public LogEntry(string level, string message)
    {
        Level   = level;
        Message = message;
    }

    /// <summary>Formatted "[level] message" line for display binding.</summary>
    public string Display => $"[{Level}] {Message}";
}
