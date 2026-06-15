// ----------------------------------------------------------------------------
//  MainWindowViewModel.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  View model for the main window. Drives port selection, firmware selection,
//  device identification, and the factory-reset flow. Protocol work runs on a
//  background thread; FactoryResetSession events are marshalled to the UI
//  thread via the Avalonia dispatcher.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Collections.ObjectModel;
using System.IO.Ports;
using Avalonia.Threading;
using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace BootloaderTool.ViewModels;

/// <summary>
/// Backs the main window: device panel, firmware panel, progress, log, and the
/// factory-reset action with error recovery.
/// </summary>
public partial class MainWindowViewModel : ViewModelBase
{
    /// <summary>Available serial ports.</summary>
    public ObservableCollection<string> Ports { get; } = new();

    /// <summary>Scrolling activity log.</summary>
    public ObservableCollection<LogEntry> Log { get; } = new();

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(ConnectCommand))]
    [NotifyCanExecuteChangedFor(nameof(FactoryResetCommand))]
    private string? _selectedPort;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(FactoryResetCommand))]
    private string? _firmwarePath;

    [ObservableProperty] private string _firmwareInfo = "No image selected.";
    [ObservableProperty] private string _deviceInfo   = "Not connected.";
    [ObservableProperty] private string _stage        = "Idle";
    [ObservableProperty] private double _progressValue;
    [ObservableProperty] private string _progressText = string.Empty;
    [ObservableProperty] private bool   _runGo        = true;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(RefreshPortsCommand))]
    [NotifyCanExecuteChangedFor(nameof(BrowseFirmwareCommand))]
    [NotifyCanExecuteChangedFor(nameof(ConnectCommand))]
    [NotifyCanExecuteChangedFor(nameof(FactoryResetCommand))]
    private bool _isBusy;

    [ObservableProperty] private bool    _hasError;
    [ObservableProperty] private string? _errorMessage;

    /// <summary>
    /// File-picker callback supplied by the view (which owns the TopLevel needed
    /// for the storage dialog). Returns the chosen path, or null if cancelled.
    /// </summary>
    public Func<Task<string?>>? FilePicker { get; set; }

    /// <summary>Creates the view model and populates the initial port list.</summary>
    public MainWindowViewModel()
    {
        RefreshPorts();
    }

    // -----------------------------------------------------------------------
    // Commands
    // -----------------------------------------------------------------------

    /// <summary>Re-enumerates serial ports, preserving the current selection.</summary>
    [RelayCommand(CanExecute = nameof(NotBusy))]
    private void RefreshPorts()
    {
        string? previous = SelectedPort;

        Ports.Clear();
        foreach (string name in SerialPort.GetPortNames().OrderBy(n => n, StringComparer.OrdinalIgnoreCase))
            Ports.Add(name);

        SelectedPort = previous is not null && Ports.Contains(previous) ? previous : Ports.FirstOrDefault();
    }

    /// <summary>Opens the file picker and loads the selected firmware image.</summary>
    [RelayCommand(CanExecute = nameof(NotBusy))]
    private async Task BrowseFirmware()
    {
        if (FilePicker is null)
            return;

        string? path = await FilePicker();
        if (!string.IsNullOrEmpty(path))
            LoadFirmware(path);
    }

    /// <summary>Connects, syncs, and reports chip ID / protocol / command count.</summary>
    [RelayCommand(CanExecute = nameof(CanConnect))]
    private async Task Connect()
    {
        string port = SelectedPort!;
        IsBusy = true;
        DeviceInfo = "Connecting...";
        try
        {
            string info = await Task.Run(() =>
            {
                using ISerial serial = new SerialPortAdapter();
                if (!serial.Open(port, new SerialConfig()))
                    throw new InvalidOperationException($"Could not open {port}.");

                var client = new An3155Client(serial);
                if (!client.Sync())
                    throw new InvalidOperationException("Bootloader did not respond (check 8E1 wiring and boot mode).");

                GetResult get = client.Get();
                ushort id     = client.GetId();
                return $"Chip 0x{id:X4}  ·  protocol v{get.ProtocolVersion >> 4}.{get.ProtocolVersion & 0x0F}" +
                       $"  ·  {get.SupportedCommands.Length} commands";
            });

            DeviceInfo = info;
        }
        catch (Exception ex)
        {
            DeviceInfo = ex.Message;
        }
        finally
        {
            IsBusy = false;
        }
    }

    /// <summary>Runs the full factory-reset flow against the selected device.</summary>
    [RelayCommand(CanExecute = nameof(CanFactoryReset))]
    private async Task FactoryReset()
    {
        HasError      = false;
        ErrorMessage  = null;
        ProgressValue = 0;
        ProgressText  = string.Empty;
        Log.Clear();
        IsBusy = true;

        string port   = SelectedPort!;
        string path   = FirmwarePath!;
        bool   runGo  = RunGo;

        try
        {
            var image = new FirmwareImage(path);

            await Task.Run(async () =>
            {
                using ISerial serial = new SerialPortAdapter();
                if (!serial.Open(port, new SerialConfig()))
                    throw new InvalidOperationException($"Could not open {port}.");

                var session = new FactoryResetSession(serial, new FactoryResetOptions { RunGo = runGo });
                session.StateChanged += state => Post(() => Stage = state.ToString());
                session.Progress     += (stage, current, total) => Post(() => UpdateProgress(stage, current, total));
                session.Log          += (level, message) => Post(() => Log.Add(new LogEntry(level, message)));

                await session.RunAsync(image);
            });

            Stage         = "Done";
            ProgressValue = 100;
            ProgressText  = runGo ? "Device booted." : "Image written and verified.";
        }
        catch (Exception ex)
        {
            HasError     = true;
            ErrorMessage = Describe(ex);
            Stage        = "Failed";
            Log.Add(new LogEntry("error", ErrorMessage));
        }
        finally
        {
            IsBusy = false;
        }
    }

    // -----------------------------------------------------------------------
    // CanExecute guards
    // -----------------------------------------------------------------------

    private bool NotBusy() => !IsBusy;

    private bool CanConnect() => !IsBusy && !string.IsNullOrEmpty(SelectedPort);

    private bool CanFactoryReset()
        => !IsBusy && !string.IsNullOrEmpty(SelectedPort) && !string.IsNullOrEmpty(FirmwarePath);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// <summary>Loads a firmware image and updates the preview, or reports failure.</summary>
    private void LoadFirmware(string path)
    {
        try
        {
            var image    = new FirmwareImage(path);
            FirmwarePath = path;
            FirmwareInfo = $"{Path.GetFileName(path)}  ·  {image.Size:N0} bytes  ·  CRC32 0x{image.Crc32:X8}";
        }
        catch (Exception ex)
        {
            FirmwarePath = null;
            FirmwareInfo = $"Cannot load image: {ex.Message}";
        }
    }

    /// <summary>Maps a (stage, current, total) event to the progress bar + caption.</summary>
    private void UpdateProgress(string stage, uint current, uint total)
    {
        Stage         = stage;
        ProgressValue = total == 0 ? 100 : current * 100.0 / total;
        ProgressText  = string.Equals(stage, "Writing", StringComparison.Ordinal)
            ? $"{current / 1024.0:F1} / {total / 1024.0:F1} KB"
            : $"{current} / {total}";
    }

    /// <summary>Turns a protocol exception into a plain-language message.</summary>
    private static string Describe(Exception ex) => ex switch
    {
        NackException nack          => $"Device rejected command 0x{nack.Command:X2} (NACK).",
        BootloaderTimeoutException  => $"Timed out waiting for the device: {ex.Message}",
        ConnectionLostException     => $"Serial connection lost and could not reconnect: {ex.Message}",
        ChecksumMismatchException c => $"Verification failed after retries: expected 0x{c.Expected:X8}, device 0x{c.Actual:X8}.",
        _                           => ex.Message,
    };

    /// <summary>Marshals an action onto the UI thread.</summary>
    private static void Post(Action action) => Dispatcher.UIThread.Post(action);
}
