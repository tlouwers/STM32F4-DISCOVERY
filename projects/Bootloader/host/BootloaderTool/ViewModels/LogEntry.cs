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
    /// <summary>When the entry was recorded (local time).</summary>
    public DateTime Timestamp { get; }

    /// <summary>Severity level (e.g. "info", "warn", "error").</summary>
    public string Level { get; }

    /// <summary>Human-readable message text.</summary>
    public string Message { get; }

    /// <summary>Creates a log entry stamped with the current local time.</summary>
    public LogEntry(string level, string message)
        : this(level, message, DateTime.Now)
    {
    }

    /// <summary>Creates a log entry with an explicit timestamp (used by tests).</summary>
    public LogEntry(string level, string message, DateTime timestamp)
    {
        Timestamp = timestamp;
        Level     = level;
        Message   = message;
    }

    /// <summary>
    /// Fixed-width "HH:mm:ss  [level]" prefix bound to its own column so wrapped
    /// message text hangs aligned under the message, not back under the timestamp.
    /// </summary>
    public string Prefix => $"{Timestamp:HH:mm:ss}  [{Level}]";

    /// <summary>True for "error" entries — drives the red log line in the view.</summary>
    public bool IsError => string.Equals(Level, "error", StringComparison.OrdinalIgnoreCase);

    /// <summary>True for "warn" entries — drives the amber log line in the view.</summary>
    public bool IsWarning => string.Equals(Level, "warn", StringComparison.OrdinalIgnoreCase);
}
