// ----------------------------------------------------------------------------
//  ConsoleProgress.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Renders FactoryResetSession progress and log events to a TextWriter. Human
//  mode draws an in-place progress bar; --json mode emits one JSON object per
//  line (JSON Lines) for scripting.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Text.Json;

namespace BootloaderTool.Cli;

/// <summary>
/// Bridges the protocol layer's <c>Progress</c> and <c>Log</c> events to a
/// text sink. In human mode it draws an in-place bar and prefixed log lines;
/// in JSON mode it writes machine-readable JSON Lines.
/// </summary>
public sealed class ConsoleProgress
{
    private const int BarWidth = 28;

    private readonly TextWriter _out;
    private readonly bool _json;
    private bool _barOpen;

    /// <summary>Creates a reporter writing to <paramref name="output"/>.</summary>
    /// <param name="output">Sink for progress and log output.</param>
    /// <param name="json">True to emit JSON Lines instead of a human bar.</param>
    public ConsoleProgress(TextWriter output, bool json)
    {
        _out  = output;
        _json = json;
    }

    /// <summary>Handles a (level, message) log event.</summary>
    public void OnLog(string level, string message)
    {
        if (_json)
        {
            Emit(new { @event = "log", level, message });
            return;
        }

        CloseBar();
        _out.WriteLine($"[{level}] {message}");
    }

    /// <summary>Handles a (stage, current, total) progress event.</summary>
    public void OnProgress(string stage, uint current, uint total)
    {
        if (_json)
        {
            Emit(new { @event = "progress", stage, current, total });
            return;
        }

        int pct = total == 0 ? 100 : (int)(current * 100L / total);
        int fill = total == 0 ? BarWidth : (int)(current * (long)BarWidth / total);
        if (fill > BarWidth) fill = BarWidth;

        string bar = new string('#', fill) + new string('-', BarWidth - fill);
        _out.Write($"\r{stage,-10} [{bar}] {pct,3}%  {current}/{total}   ");
        _barOpen = true;

        if (current >= total)
            CloseBar();
    }

    /// <summary>Reports a terminal error for the operation.</summary>
    public void OnError(string message)
    {
        if (_json)
        {
            Emit(new { @event = "error", message });
            return;
        }

        CloseBar();
        _out.WriteLine($"[error] {message}");
    }

    /// <summary>Finishes the current bar line so later output starts cleanly.</summary>
    private void CloseBar()
    {
        if (_barOpen)
        {
            _out.WriteLine();
            _barOpen = false;
        }
    }

    private void Emit(object payload) => _out.WriteLine(JsonSerializer.Serialize(payload));
}
