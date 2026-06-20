// ----------------------------------------------------------------------------
//  SessionLog.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Append-only file logger for the GUI. Every line that reaches the on-screen
//  activity log is also written here so a flash can be inspected after the fact.
//  The file lives beside the application binary (bootloader.log) and is only ever
//  appended to — never overwritten — with each flash run separated by a rule.
//  File-I/O failures are swallowed: persistent logging must never break the app.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Logging;

/// <summary>
/// Thread-safe, append-only logger that mirrors the activity log to a file next
/// to the executable. Best-effort: any I/O error is ignored so logging never
/// disrupts the UI.
/// </summary>
public sealed class SessionLog
{
    private static readonly string Separator = new('-', 60);

    private readonly object _gate = new();
    private readonly string _path;

    /// <summary>Creates a logger writing to "bootloader.log" beside the binary.</summary>
    public SessionLog()
        : this(System.IO.Path.Combine(AppContext.BaseDirectory, "bootloader.log"))
    {
    }

    /// <summary>Creates a logger writing to an explicit path (used by tests).</summary>
    public SessionLog(string path) => _path = path;

    /// <summary>Full path of the log file on disk.</summary>
    public string Path => _path;

    /// <summary>
    /// Writes a blank line and a horizontal rule, separating one flash run's lines
    /// from the next in the append-only file.
    /// </summary>
    public void WriteSeparator()
    {
        lock (_gate)
            TryAppend(Environment.NewLine + Separator + Environment.NewLine);
    }

    /// <summary>Appends one timestamped "[level] message" line.</summary>
    /// <param name="level">Severity tag (e.g. "info", "error").</param>
    /// <param name="message">The message text.</param>
    public void Write(string level, string message)
    {
        string line = $"{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff}  [{level}] {message}{Environment.NewLine}";
        lock (_gate)
            TryAppend(line);
    }

    private void TryAppend(string text)
    {
        try
        {
            File.AppendAllText(_path, text);
        }
        catch
        {
            // Logging is best-effort: a locked/unwritable file must not break a flash.
        }
    }
}
