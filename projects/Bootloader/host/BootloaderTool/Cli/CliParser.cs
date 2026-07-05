// ----------------------------------------------------------------------------
//  CliParser.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Hand-rolled command-line parser. Splits argv into a verb plus a populated
//  CliOptions, or reports a usage error / help request. Dependency-free so the
//  tool builds offline and the parser is trivially unit-testable.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Globalization;

namespace BootloaderTool.Cli;

/// <summary>
/// Outcome of parsing the command line: the verb, the populated options, and
/// either a usage error or a help request. Exactly one of (<see cref="Error"/>,
/// <see cref="ShowHelp"/>, normal verb) is the meaningful result.
/// </summary>
public sealed class ParseResult
{
    /// <summary>The command verb, or null when only top-level help was asked for.</summary>
    public string? Verb { get; set; }

    /// <summary>Parsed options (defaults when not supplied).</summary>
    public CliOptions Options { get; set; } = new();

    /// <summary>Usage error message, or null when parsing succeeded.</summary>
    public string? Error { get; set; }

    /// <summary>True when help was requested (no args, or -h/--help).</summary>
    public bool ShowHelp { get; set; }
}

/// <summary>
/// Parses argv into a <see cref="ParseResult"/>. Knows the option grammar but
/// not the per-command semantics — required-option checks live in each command.
/// </summary>
public static class CliParser
{
    /// <summary>Parses the raw argument vector.</summary>
    public static ParseResult Parse(string[] args)
    {
        var result = new ParseResult();

        if (args.Length == 0 || IsHelp(args[0]))
        {
            result.ShowHelp = true;
            return result;
        }

        result.Verb = args[0];
        CliOptions o = result.Options;

        for (int i = 1; i < args.Length; i++)
        {
            string a = args[i];

            if (IsHelp(a))
            {
                result.ShowHelp = true;
                continue;
            }

            switch (a)
            {
                case "-p":
                case "--port":
                    if (!TakeValue(args, ref i, out string port)) return Fail(result, $"missing value for {a}");
                    o.Port = port;
                    break;

                case "-f":
                case "--file":
                    if (!TakeValue(args, ref i, out string file)) return Fail(result, $"missing value for {a}");
                    o.File = file;
                    break;

                case "-o":
                case "--output":
                    if (!TakeValue(args, ref i, out string output)) return Fail(result, $"missing value for {a}");
                    o.Output = output;
                    break;

                case "--addr":
                    if (!TakeValue(args, ref i, out string addr)) return Fail(result, $"missing value for {a}");
                    if (!TryParseUInt(addr, out uint addrVal)) return Fail(result, $"invalid address '{addr}'");
                    o.Address = addrVal;
                    break;

                case "--len":
                    if (!TakeValue(args, ref i, out string len)) return Fail(result, $"missing value for {a}");
                    if (!TryParseInt(len, out int lenVal) || lenVal <= 0) return Fail(result, $"invalid length '{len}'");
                    o.Length = lenVal;
                    break;

                case "--baud":
                    if (!TakeValue(args, ref i, out string baud)) return Fail(result, $"missing value for {a}");
                    if (!TryParseInt(baud, out int baudVal) || baudVal <= 0) return Fail(result, $"invalid baud '{baud}'");
                    o.Baud = baudVal;
                    break;

                case "--retries":
                    if (!TakeValue(args, ref i, out string retries)) return Fail(result, $"missing value for {a}");
                    if (!TryParseInt(retries, out int retriesVal) || retriesVal < 0) return Fail(result, $"invalid retries '{retries}'");
                    o.Retries = retriesVal;
                    break;

                case "--timeout":
                    if (!TakeValue(args, ref i, out string timeout)) return Fail(result, $"missing value for {a}");
                    if (!TryParseInt(timeout, out int timeoutVal) || timeoutVal <= 0) return Fail(result, $"invalid timeout '{timeout}'");
                    o.TimeoutMs = timeoutVal;
                    break;

                case "--no-go":
                    o.NoGo = true;
                    break;

                case "--force":
                    o.Force = true;
                    break;

                case "--json":
                    o.Json = true;
                    break;

                default:
                    return Fail(result,
                        a.StartsWith('-') ? $"unknown option '{a}'" : $"unexpected argument '{a}'");
            }
        }

        return result;
    }

    /// <summary>Parses an unsigned integer in decimal or <c>0x</c> hex form.</summary>
    public static bool TryParseUInt(string text, out uint value)
    {
        text = text.Trim();
        if (text.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            return uint.TryParse(text.AsSpan(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out value);
        return uint.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out value);
    }

    /// <summary>Parses a signed integer in decimal or <c>0x</c> hex form.</summary>
    public static bool TryParseInt(string text, out int value)
    {
        text = text.Trim();
        if (text.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
        {
            if (uint.TryParse(text.AsSpan(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out uint u)
                && u <= int.MaxValue)
            {
                value = (int)u;
                return true;
            }
            value = 0;
            return false;
        }
        return int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out value);
    }

    private static bool IsHelp(string a) => a is "-h" or "--help";

    private static bool TakeValue(string[] args, ref int i, out string value)
    {
        if (i + 1 >= args.Length)
        {
            value = string.Empty;
            return false;
        }
        value = args[++i];
        return true;
    }

    private static ParseResult Fail(ParseResult result, string error)
    {
        result.Error = error;
        return result;
    }
}
