// ----------------------------------------------------------------------------
//  SystemSerialPortEnumerator.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Cross-platform serial-port enumerator. Lists ports via
//  SerialPort.GetPortNames() everywhere, and on Windows enriches each entry with
//  a friendly description (Win32_PnPEntity via WMI) so USB-UART adapters can be
//  identified and preferred. On non-Windows hosts the port name itself
//  (/dev/ttyUSB*, /dev/ttyACM*) is used for the USB-UART heuristic.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.IO.Ports;
using System.Management;
using System.Runtime.Versioning;
using System.Text.RegularExpressions;

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// Default <see cref="ISerialPortEnumerator"/> backed by the OS. Safe to call
/// from a background poll loop; never throws for a missing description, and lets
/// transient enumeration failures surface to the caller (the watcher swallows
/// them as "no change").
/// </summary>
public sealed class SystemSerialPortEnumerator : ISerialPortEnumerator
{
    // Matches the "(COM3)" suffix Windows appends to a device's friendly name.
    private static readonly Regex ComPortPattern = new(@"\(COM(\d+)\)", RegexOptions.Compiled);
    private static readonly Regex ComPortSuffix  = new(@"\s*\(COM\d+\)\s*$", RegexOptions.Compiled);

    /// <inheritdoc />
    public IReadOnlyList<SerialPortInfo> Enumerate()
    {
        string[] rawNames = SerialPort.GetPortNames();

        // GetPortNames() is known to occasionally return names with a trailing
        // NUL or whitespace; normalise and drop empties before use.
        List<string> names = rawNames
            .Select(n => n.TrimEnd('\0').Trim())
            .Where(n => n.Length > 0)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderBy(n => n, StringComparer.OrdinalIgnoreCase)
            .ToList();

        IReadOnlyDictionary<string, string> descriptions = OperatingSystem.IsWindows()
            ? QueryWindowsDescriptions()
            : EmptyDescriptions;

        var ports = new List<SerialPortInfo>(names.Count);
        foreach (string name in names)
        {
            descriptions.TryGetValue(name, out string? description);
            ports.Add(new SerialPortInfo(name, description));
        }

        return ports;
    }

    private static readonly IReadOnlyDictionary<string, string> EmptyDescriptions =
        new Dictionary<string, string>();

    /// <summary>
    /// Builds a COM-port → friendly-name map from Win32_PnPEntity. Returns an
    /// empty map if WMI is unavailable or access is denied.
    /// </summary>
    [SupportedOSPlatform("windows")]
    private static IReadOnlyDictionary<string, string> QueryWindowsDescriptions()
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT Name FROM Win32_PnPEntity WHERE Name LIKE '%(COM%'");

            foreach (ManagementBaseObject device in searcher.Get())
            {
                using (device)
                {
                    if (device["Name"] is not string fullName)
                        continue;

                    Match match = ComPortPattern.Match(fullName);
                    if (!match.Success)
                        continue;

                    string port        = "COM" + match.Groups[1].Value;
                    string description = ComPortSuffix.Replace(fullName, string.Empty).Trim();
                    map[port] = description.Length > 0 ? description : fullName;
                }
            }
        }
        catch
        {
            // WMI not available (e.g. service stopped) or access denied — fall
            // back to bare port names. Enumeration must never fail for this.
        }
        return map;
    }
}
