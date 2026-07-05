// ----------------------------------------------------------------------------
//  SerialPortInfo.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Describes a serial port discovered on the host: its system name, an optional
//  human-readable description, and a ranked guess at how strongly it looks like a
//  USB-UART bridge (so the UI can auto-pick the real cable over a generic CDC).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// A serial port discovered on the host. <see cref="Description"/> is best-effort
/// (populated on Windows via WMI; null elsewhere). <see cref="MatchRank"/> scores
/// how strongly the port looks like the USB-UART bridge we want, so the UI can
/// auto-pick a real cable ("USB Serial Port", FTDI, CP210x, /dev/ttyUSB*, …) over
/// a board's generic virtual COM port ("Serieel USB-apparaat", a bare CDC device).
/// </summary>
public sealed class SerialPortInfo
{
    // Names specific to USB-UART bridge chips/drivers — the cable we want (rank 2).
    private static readonly string[] StrongUsbUartHints =
    {
        "USB SERIAL PORT", "USB-SERIAL", "UART", "CP210", "CH340", "CH910",
        "PL2303", "PROLIFIC", "FTDI", "FT232", "SILICON LABS",
    };

    // Generic / localised USB-serial CDC names — present, but not preferred (rank 1).
    private static readonly string[] GenericUsbHints =
    {
        "USB", "SERIAL", "SERIEEL", "SERIELL", "SÉRIE", "SERIE",
    };

    /// <summary>Creates a descriptor for a discovered port.</summary>
    /// <param name="portName">System port name (e.g. "COM3", "/dev/ttyUSB0").</param>
    /// <param name="description">Friendly description, or null if unknown.</param>
    public SerialPortInfo(string portName, string? description)
    {
        PortName    = portName;
        Description = description;
        MatchRank   = ComputeMatchRank(portName, description);
    }

    /// <summary>System port name (e.g. "COM3", "/dev/ttyUSB0").</summary>
    public string PortName { get; }

    /// <summary>Friendly description, or null when the host cannot supply one.</summary>
    public string? Description { get; }

    /// <summary>
    /// How strongly this port looks like the USB-UART bridge to flash through —
    /// higher wins. 0 = not a candidate, 1 = a generic USB CDC device, 2 = a
    /// recognised USB-UART bridge by chip/driver name (or a USB device path on
    /// Linux/macOS). The UI auto-selects the single highest-ranked port. Computed
    /// once in the constructor — the inputs are immutable.
    /// </summary>
    public int MatchRank { get; }

    /// <summary>Scores how strongly the port looks like the USB-UART bridge we want.</summary>
    private static int ComputeMatchRank(string portName, string? description)
    {
        // Linux/macOS expose USB serial devices by path, with no description.
        string name = portName.ToUpperInvariant();
        if (name.Contains("USB") || name.Contains("ACM"))
            return 2;

        if (string.IsNullOrEmpty(description))
            return 0;

        string desc = description!.ToUpperInvariant();
        foreach (string hint in StrongUsbUartHints)
        {
            if (desc.Contains(hint))
                return 2;
        }
        foreach (string hint in GenericUsbHints)
        {
            if (desc.Contains(hint))
                return 1;
        }
        return 0;
    }

    /// <summary>True when the port looks like a USB-UART adapter (any rank above 0).</summary>
    public bool IsLikelyUsbUart => MatchRank > 0;

    /// <summary>Port name plus description, for display in a list.</summary>
    public string DisplayName => string.IsNullOrEmpty(Description)
        ? PortName
        : $"{PortName} — {Description}";

    /// <inheritdoc />
    public override string ToString() => DisplayName;
}
