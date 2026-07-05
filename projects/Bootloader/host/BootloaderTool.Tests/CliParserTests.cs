// ----------------------------------------------------------------------------
//  CliParserTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for CliParser: verb extraction, option/value parsing, hex and
//  decimal numbers, help requests, and usage errors.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Cli;
using Xunit;

namespace BootloaderTool.Tests;

public class CliParserTests
{
    [Fact]
    public void Parse_NoArgs_RequestsHelp()
    {
        ParseResult result = CliParser.Parse(Array.Empty<string>());

        Assert.True(result.ShowHelp);
        Assert.Null(result.Verb);
        Assert.Null(result.Error);
    }

    [Theory]
    [InlineData("-h")]
    [InlineData("--help")]
    public void Parse_TopLevelHelpFlag_RequestsHelp(string flag)
    {
        ParseResult result = CliParser.Parse(new[] { flag });

        Assert.True(result.ShowHelp);
        Assert.Null(result.Verb);
    }

    [Fact]
    public void Parse_CommandHelpFlag_KeepsVerbAndRequestsHelp()
    {
        ParseResult result = CliParser.Parse(new[] { "factory-reset", "--help" });

        Assert.True(result.ShowHelp);
        Assert.Equal("factory-reset", result.Verb);
    }

    [Fact]
    public void Parse_FullOptionSet_PopulatesOptions()
    {
        ParseResult result = CliParser.Parse(new[]
        {
            "factory-reset",
            "-p", "COM7",
            "-f", "fw.bin",
            "--addr", "0x08000000",
            "--baud", "57600",
            "--timeout", "3000",
            "--retries", "5",
            "--no-go",
            "--json",
        });

        Assert.Null(result.Error);
        Assert.Equal("factory-reset", result.Verb);
        CliOptions o = result.Options;
        Assert.Equal("COM7", o.Port);
        Assert.Equal("fw.bin", o.File);
        Assert.Equal(0x08000000u, o.Address);
        Assert.Equal(57600, o.Baud);
        Assert.Equal(3000, o.TimeoutMs);
        Assert.Equal(5, o.Retries);
        Assert.True(o.NoGo);
        Assert.True(o.Json);
    }

    [Fact]
    public void Parse_LongAndShortAliases_AreEquivalent()
    {
        ParseResult shortForm = CliParser.Parse(new[] { "read", "-p", "COM1", "-o", "out.bin" });
        ParseResult longForm  = CliParser.Parse(new[] { "read", "--port", "COM1", "--output", "out.bin" });

        Assert.Equal(shortForm.Options.Port,   longForm.Options.Port);
        Assert.Equal(shortForm.Options.Output, longForm.Options.Output);
    }

    [Theory]
    [InlineData("0x400", 1024)]
    [InlineData("1024", 1024)]
    public void Parse_Length_AcceptsHexAndDecimal(string text, int expected)
    {
        ParseResult result = CliParser.Parse(new[] { "read", "--len", text });

        Assert.Null(result.Error);
        Assert.Equal(expected, result.Options.Length);
    }

    [Fact]
    public void Parse_UnknownOption_ReportsError()
    {
        ParseResult result = CliParser.Parse(new[] { "info", "--bogus" });

        Assert.NotNull(result.Error);
        Assert.Contains("unknown option", result.Error);
    }

    [Fact]
    public void Parse_MissingOptionValue_ReportsError()
    {
        ParseResult result = CliParser.Parse(new[] { "info", "-p" });

        Assert.NotNull(result.Error);
        Assert.Contains("missing value", result.Error);
    }

    [Fact]
    public void Parse_InvalidAddress_ReportsError()
    {
        ParseResult result = CliParser.Parse(new[] { "go", "--addr", "nothex" });

        Assert.NotNull(result.Error);
        Assert.Contains("invalid address", result.Error);
    }

    [Fact]
    public void Parse_NonPositiveLength_ReportsError()
    {
        ParseResult result = CliParser.Parse(new[] { "read", "--len", "0" });

        Assert.NotNull(result.Error);
        Assert.Contains("invalid length", result.Error);
    }

    [Fact]
    public void Parse_UnexpectedPositional_ReportsError()
    {
        ParseResult result = CliParser.Parse(new[] { "info", "stray" });

        Assert.NotNull(result.Error);
        Assert.Contains("unexpected argument", result.Error);
    }
}
