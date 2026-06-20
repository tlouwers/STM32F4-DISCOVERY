// ----------------------------------------------------------------------------
//  MainWindowViewModel.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  View model for the main window. Serial ports are discovered and kept current
//  by a background SerialPortWatcher (insert/remove safe); the selected port is
//  auto-probed for a live bootloader so the Flash Firmware action only lights up
//  when it can actually succeed. Exposes a small status model (title + dot state)
//  for the header strip and step-1 card. Protocol work runs on background
//  threads, marshalled back to the UI thread via the Avalonia dispatcher.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 3.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Collections.ObjectModel;
using Avalonia.Threading;
using BootloaderTool.Logging;
using BootloaderTool.Protocol.Protocol;
using BootloaderTool.Protocol.Serial;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace BootloaderTool.ViewModels;

/// <summary>
/// Backs the main window: live device panel (auto-detect + auto-probe), firmware
/// panel (browse + drag-and-drop), the flash action with progress, the activity
/// log, and an error/recovery banner.
/// </summary>
public partial class MainWindowViewModel : ViewModelBase, IDisposable
{
    /// <summary>
    /// A live, already-synced bootloader connection. The auto-probe opens the
    /// port, sends the one-shot 0x7F sync, identifies the device, and then keeps
    /// the port open inside one of these so the flash can reuse the same session —
    /// the STM32F4 ROM bootloader ACKs 0x7F only once per entry, so a second sync
    /// (re-open + re-sync) would fail.
    /// </summary>
    private sealed class LiveConnection : IDisposable
    {
        public LiveConnection(ISerial serial, An3155Client client)
        {
            Serial = serial;
            Client = client;
        }

        /// <summary>The open serial port (owned — disposed with the connection).</summary>
        public ISerial Serial { get; }

        /// <summary>A client already synced and identified against the device.</summary>
        public An3155Client Client { get; }

        public void Dispose() => Serial.Dispose();
    }

    /// <summary>Result of a one-shot bootloader probe on a port.</summary>
    private readonly record struct ProbeResult(bool Success, string Info, LiveConnection? Connection = null)
    {
        public static ProbeResult Ok(string info, LiveConnection connection) => new(true, info, connection);
        public static ProbeResult Fail(string info)                          => new(false, info);
    }

    /// <summary>How long to wait before re-probing a port that has no bootloader yet.</summary>
    private static readonly TimeSpan ProbeRetryDelay = TimeSpan.FromSeconds(2);

    /// <summary>Short serial timeout used for probing (a single ACK byte is quick).</summary>
    private static readonly SerialConfig ProbeConfig = new() { TimeoutMs = 500 };

    /// <summary>Read/write timeout restored on the held port before a flash (longer command ACKs).</summary>
    private const int FlashTimeoutMs = 2000;

    private readonly SerialPortWatcher _watcher;
    private CancellationTokenSource? _probeCts;

    /// <summary>Cancels an in-flight flash (used on shutdown to stop writing cleanly).</summary>
    private CancellationTokenSource? _flashCts;

    /// <summary>Live bootloader connection held between probe and flash; null when not connected.</summary>
    private LiveConnection? _connection;

    /// <summary>Append-only file log mirroring the on-screen activity log.</summary>
    private readonly SessionLog _sessionLog = new();

    /// <summary>Available serial ports, kept current by the watcher.</summary>
    public ObservableCollection<SerialPortInfo> Ports { get; } = new();

    /// <summary>Scrolling activity log.</summary>
    public ObservableCollection<LogEntry> Log { get; } = new();

    /// <summary>True when the activity log has at least one entry.</summary>
    public bool HasLog => Log.Count > 0;

    /// <summary>True when the folded "Activity Log" bar should show (log exists, collapsed).</summary>
    public bool ShowActivityBar => HasLog && !ActivityExpanded;

    /// <summary>True when the expanded activity overlay should show (log exists, expanded).</summary>
    public bool ShowActivitySheet => HasLog && ActivityExpanded;

    /// <summary>
    /// True only when more than one port is available, so the user has a real
    /// choice to make. With zero or one port the chooser is hidden and the card
    /// simply reflects the connection status.
    /// </summary>
    public bool ShowPortChooser => Ports.Count > 1;

    /// <summary>True when a flash can run right now (drives the action + its caption).</summary>
    public bool IsReadyToFlash => CanFlashFirmware();

    /// <summary>
    /// True once a valid image is loaded. Switches card 2 from its empty drop
    /// zone to the file + statistics view, independently of the connection state
    /// (the statistics stay visible right up to and through a flash).
    /// </summary>
    public bool HasFirmware => !string.IsNullOrEmpty(FirmwarePath);

    /// <summary>
    /// Single always-present line under the flash button (kept visible in every
    /// state so the card never changes height): names what is still missing while
    /// disabled, confirms readiness when armed, and reassures while flashing.
    /// </summary>
    public string FlashHint
        => IsBusy                          ? "Flashing — keep the cable connected until it finishes."
            : !DeviceDetected && !HasFirmware ? "Connect a device and select a .bin image to enable flashing."
            : !DeviceDetected              ? "Connect a device to enable flashing."
            : !HasFirmware                 ? "Select a .bin image to enable flashing."
            : "Ready to flash — press Flash firmware.";

    /// <summary>
    /// Flash button caption. Switches to "In progress…" during a flash so the
    /// button stays in place (greyed by its disabled state) instead of being
    /// hidden — which would shrink card 3 and shift the activity bar.
    /// </summary>
    public string FlashButtonText => IsBusy ? "In progress…" : "Flash firmware";

    /// <summary>Largest image the tool will accept (matches the ".bin — up to 2 MB" hint).</summary>
    private const long MaxFirmwareBytes = 2L * 1024 * 1024;

    [ObservableProperty]
    private SerialPortInfo? _selectedPort;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(FlashFirmwareCommand))]
    private string? _firmwarePath;

    // -- Firmware panel (empty state vs. selected/split state) ----------------
    [ObservableProperty] private string  _firmwareName     = string.Empty;
    [ObservableProperty] private string  _firmwareSizeText = string.Empty;
    [ObservableProperty] private string  _firmwareCrcText  = string.Empty;
    [ObservableProperty] private string? _firmwareError;

    /// <summary>True when the last load attempt was rejected (drives the inline notice).</summary>
    public bool HasFirmwareError => !string.IsNullOrEmpty(FirmwareError);

    [ObservableProperty] private string _deviceInfo   = "Searching for a device...";
    [ObservableProperty] private string _stage        = "Idle";
    [ObservableProperty] private double _progressValue;
    [ObservableProperty] private string _progressText = string.Empty;
    [ObservableProperty] private bool   _runGo        = true;

    // -- Connection stoplight (card 1): green connected, red not, spinner busy --
    [ObservableProperty] private string _statusTitle = "Not connected";
    [ObservableProperty] private bool   _isConnectedState;
    [ObservableProperty] private bool   _isDisconnectedState = true;

    /// <summary>True while a flash is running (gates all user actions; shows progress).</summary>
    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(BrowseFirmwareCommand))]
    [NotifyCanExecuteChangedFor(nameof(FlashFirmwareCommand))]
    private bool _isBusy;

    /// <summary>True while a background probe is identifying the selected port.</summary>
    [ObservableProperty] private bool _isProbing;

    /// <summary>True when the last probe found a live bootloader on the selected port.</summary>
    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(FlashFirmwareCommand))]
    private bool _deviceDetected;

    [ObservableProperty] private bool    _hasError;
    [ObservableProperty] private string? _errorMessage;

    /// <summary>
    /// Whether the activity log overlay is unfolded (true) or shown as the folded
    /// "Activity Log" bar (false). Starts folded; the log appears only after a flash
    /// produces entries.
    /// </summary>
    [ObservableProperty] private bool _activityExpanded;

    // True while the current SelectedPort was chosen automatically (not by the
    // user). An auto-selection may be upgraded if a better-ranked cable appears;
    // a manual one is left alone. Set only via the programmatic-assignment guard.
    private bool _portChosenAutomatically;

    // Guards programmatic SelectedPort assignment so OnSelectedPortChanged can
    // tell an auto-pick from a user pick.
    private bool _settingPortProgrammatically;

    /// <summary>
    /// File-picker callback supplied by the view (which owns the TopLevel needed
    /// for the storage dialog). Returns the chosen path, or null if cancelled.
    /// </summary>
    public Func<Task<string?>>? FilePicker { get; set; }

    /// <summary>Creates the view model with the default OS port watcher.</summary>
    public MainWindowViewModel()
        : this(new SerialPortWatcher(new SystemSerialPortEnumerator()))
    {
    }

    /// <summary>Creates the view model with an injected watcher (used by tests).</summary>
    public MainWindowViewModel(SerialPortWatcher watcher)
    {
        _watcher = watcher;
        _watcher.PortsChanged += OnPortsChanged;
        Log.CollectionChanged += (_, _) =>
        {
            OnPropertyChanged(nameof(HasLog));
            OnPropertyChanged(nameof(ShowActivityBar));
            OnPropertyChanged(nameof(ShowActivitySheet));
        };
        Ports.CollectionChanged += (_, _) => OnPropertyChanged(nameof(ShowPortChooser));
        _watcher.Start();
    }

    // -----------------------------------------------------------------------
    // Commands
    // -----------------------------------------------------------------------

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

    /// <summary>Flashes the selected image to the connected device (erase, write, verify, optional Go).</summary>
    [RelayCommand(CanExecute = nameof(CanFlashFirmware))]
    private async Task FlashFirmware()
    {
        HasError      = false;
        ErrorMessage  = null;
        ProgressValue = 0;
        ProgressText  = string.Empty;
        Log.Clear();
        IsBusy = true;

        // Stop the probe loop but keep the live connection: the flash reuses the
        // already-synced session, so the one-shot 0x7F sync is sent only once.
        CancelProbe();

        var cts = new CancellationTokenSource();
        _flashCts = cts;

        LiveConnection? connection = _connection;
        string path  = FirmwarePath!;
        string name  = FirmwareName;
        bool   runGo = RunGo;

        // Separate this run from the previous one in the append-only file, then
        // record exactly which image we are flashing (handy when reading back the
        // log later — the on-screen log is cleared each run, the file is not).
        _sessionLog.WriteSeparator();
        AppendLog("info", $"Flashing firmware: {name}");
        AppendLog("info", $"Source path: {path}");

        bool succeeded        = false;
        bool connectionUsable = false;
        try
        {
            if (connection is null)
                throw new InvalidOperationException(
                    "Lost the bootloader connection — re-enter the bootloader (RESET, then the blue button) and try again.");

            var image = new FirmwareImage(path);

            await Task.Run(async () =>
            {
                // The probe opened the port with a short timeout; restore the
                // longer flash default for erase/write/verify command ACKs.
                connection.Serial.SetTimeout(FlashTimeoutMs);

                var session = new FactoryResetSession(connection.Client, new FactoryResetOptions { RunGo = runGo });
                session.StateChanged += state => Post(() => Stage = state.ToString());
                session.Progress     += (stage, current, total) => Post(() => UpdateProgress(stage, current, total));
                session.Log          += (level, message) => Post(() => AppendLog(level, message));

                await session.RunAsync(image, cts.Token);
            }, cts.Token);

            Stage         = "Done";
            ProgressValue = 100;
            ProgressText  = runGo ? "Device booted." : "Image written and verified.";
            AppendLog("info", runGo ? "Done — device booted." : "Done — image written and verified.");
            succeeded     = true;

            // Clear the image so the next flash is a deliberate re-selection,
            // keeping the user in the loop rather than silently re-flashing.
            ClearFirmwareSelection();
        }
        catch (Exception ex)
        {
            HasError     = true;
            ErrorMessage = Describe(ex);
            Stage        = "Failed";
            AppendLog("error", ErrorMessage);

            // A failure that did not lose the link leaves the bootloader in
            // command mode, so the held connection is still usable — keep it so
            // the error banner's Retry can fire at once instead of waiting for a
            // fresh probe to re-detect the device.
            connectionUsable = _connection is not null && ex is not ConnectionLostException;
        }
        finally
        {
            IsBusy = false;
            if (cts == _flashCts)
            {
                _flashCts = null;
                cts.Dispose();
            }

            if (succeeded || !connectionUsable)
                // Success (after Go the device has left the bootloader) or a lost
                // link: re-probe to refresh detection and the held connection.
                RestartProbe();
            else
                // Keep the live connection and the "Connected" state intact so
                // Retry is immediately actionable.
                IsProbing = false;
        }
    }

    /// <summary>Unfolds or folds the activity log overlay.</summary>
    [RelayCommand]
    private void ToggleActivity() => ActivityExpanded = !ActivityExpanded;

    partial void OnActivityExpandedChanged(bool value)
    {
        OnPropertyChanged(nameof(ShowActivityBar));
        OnPropertyChanged(nameof(ShowActivitySheet));
    }

    // -----------------------------------------------------------------------
    // CanExecute guards
    // -----------------------------------------------------------------------

    private bool NotBusy() => !IsBusy;

    private bool CanFlashFirmware()
        => !IsBusy && DeviceDetected && !string.IsNullOrEmpty(FirmwarePath);

    // -----------------------------------------------------------------------
    // Header status model
    // -----------------------------------------------------------------------

    partial void OnIsBusyChanged(bool value)
    {
        OnPropertyChanged(nameof(IsReadyToFlash));
        OnPropertyChanged(nameof(FlashButtonText));
        OnPropertyChanged(nameof(FlashHint));
    }

    partial void OnDeviceDetectedChanged(bool value)
    {
        RefreshStatus();
        OnPropertyChanged(nameof(IsReadyToFlash));
        OnPropertyChanged(nameof(FlashHint));
    }

    partial void OnIsProbingChanged(bool value)      => RefreshStatus();

    partial void OnFirmwareErrorChanged(string? value) => OnPropertyChanged(nameof(HasFirmwareError));

    partial void OnFirmwarePathChanged(string? value)
    {
        OnPropertyChanged(nameof(IsReadyToFlash));
        OnPropertyChanged(nameof(HasFirmware));
        OnPropertyChanged(nameof(FlashHint));
    }

    /// <summary>
    /// Recomputes the connection stoplight. It reflects the link only — green
    /// when a bootloader is present, red when it is not, and the spinner (bound
    /// to <see cref="IsProbing"/>) while connecting. Flash progress and failures
    /// live in card 3, so they never recolour this dot; a device stays "Connected"
    /// throughout a flash.
    /// </summary>
    private void RefreshStatus()
    {
        bool connected = DeviceDetected;

        IsConnectedState    = connected;
        IsDisconnectedState = !connected && !IsProbing;

        StatusTitle = connected ? "Connected"
            : IsProbing         ? "Connecting..."
            : "Not connected";
    }

    // -----------------------------------------------------------------------
    // Port watching (insert / remove)
    // -----------------------------------------------------------------------

    /// <summary>Marshals a watcher event onto the UI thread.</summary>
    private void OnPortsChanged(object? sender, SerialPortsChangedEventArgs e) => Post(() => ApplyPortChanges(e));

    /// <summary>
    /// Reconciles the <see cref="Ports"/> collection with a watcher delta,
    /// preserving the current selection, dropping a vanished selection, and
    /// auto-selecting a port when none is chosen.
    /// </summary>
    private void ApplyPortChanges(SerialPortsChangedEventArgs e)
    {
        // Note whether our selection is being removed before we mutate Ports:
        // removing the bound item nulls SelectedItem synchronously via the
        // ComboBox binding, so we cannot read it reliably afterwards.
        SerialPortInfo? selected = SelectedPort;
        bool selectionLost = selected is not null && e.Removed.Any(r => Same(r, selected));

        foreach (SerialPortInfo removed in e.Removed)
        {
            // While a flash owns the port, keep the in-use port in the list:
            // removing it nulls SelectedPort via the ComboBox binding, which
            // would tear down (and dispose) the live connection mid-write. The
            // flash's finally reconciles everything once it completes.
            if (IsBusy && selected is not null && Same(removed, selected))
                continue;

            SerialPortInfo? existing = Ports.FirstOrDefault(p => Same(p, removed));
            if (existing is not null)
                Ports.Remove(existing);
        }

        foreach (SerialPortInfo added in e.Added)
        {
            if (!Ports.Any(p => Same(p, added)))
                Ports.Add(added);
        }

        // Selected port gone (cable yanked): clear it. Removing the bound item
        // may already have nulled the selection; this keeps state consistent.
        // Never while flashing — the flash holds the port for the session.
        if (!IsBusy && SelectedPort is not null && !Ports.Any(p => Same(p, SelectedPort)))
            SelectedPort = null;

        if (selectionLost && !IsBusy)
            DeviceInfo = $"{selected!.PortName} disconnected.";

        // Auto-select while not connected, preferring the most cable-like port.
        // This re-runs on every change so a better-ranked cable that enumerates
        // *after* a generic CDC (which would otherwise get locked in as the lone
        // port) still wins. A held connection or a port the user picked by hand
        // is never overridden.
        if (!IsBusy && !DeviceDetected && (SelectedPort is null || _portChosenAutomatically))
        {
            if (Ports.Count == 0)
            {
                SelectedPort = null;
                _portChosenAutomatically = false;
                DeviceInfo = "No serial ports found. Plug in the USB-UART cable.";
            }
            else
            {
                SerialPortInfo? best = ChooseDefaultPort();
                if (best is not null)
                {
                    if (SelectedPort is null || !Same(best, SelectedPort))
                        SetPortAutomatically(best);
                }
                else if (SelectedPort is null)
                {
                    DeviceInfo = "Multiple ports found — pick the USB-UART cable below.";
                }
            }
        }
    }

    /// <summary>
    /// Assigns <see cref="SelectedPort"/> as an automatic choice, marking it so a
    /// later, better-ranked port may upgrade it. The guard lets
    /// <see cref="OnSelectedPortChanged"/> distinguish this from a user pick.
    /// </summary>
    private void SetPortAutomatically(SerialPortInfo port)
    {
        _settingPortProgrammatically = true;
        _portChosenAutomatically = true;
        SelectedPort = port;
        _settingPortProgrammatically = false;
    }

    /// <summary>
    /// Picks the port to auto-select, or null to leave the choice to the user.
    /// A lone port is taken as-is; otherwise the single highest-ranked USB-UART
    /// candidate wins. A tie at the top (or no candidate at all) returns null so
    /// the chooser stays open rather than guessing — e.g. an ST-Link virtual COM
    /// ("Serieel USB-apparaat") never outranks the FTDI cable ("USB Serial Port").
    /// </summary>
    private SerialPortInfo? ChooseDefaultPort()
    {
        if (Ports.Count == 1)
            return Ports[0];

        int top = Ports.Max(p => p.MatchRank);
        if (top == 0)
            return null;

        var leaders = Ports.Where(p => p.MatchRank == top).ToList();
        return leaders.Count == 1 ? leaders[0] : null;
    }

    private static bool Same(SerialPortInfo a, SerialPortInfo b)
        => string.Equals(a.PortName, b.PortName, StringComparison.OrdinalIgnoreCase);

    // -----------------------------------------------------------------------
    // Auto-probe
    // -----------------------------------------------------------------------

    /// <summary>Restarts the probe loop whenever the selected port changes.</summary>
    partial void OnSelectedPortChanged(SerialPortInfo? value)
    {
        // A change that did not come through SetPortAutomatically is the user's
        // own choice — pin it so auto-upgrade leaves it alone.
        if (!_settingPortProgrammatically)
            _portChosenAutomatically = false;

        RestartProbe();
    }

    /// <summary>Cancels any running probe loop and frees its cancellation source.</summary>
    private void CancelProbe()
    {
        _probeCts?.Cancel();
        _probeCts?.Dispose();
        _probeCts = null;
    }

    /// <summary>Closes and forgets the held bootloader connection, if any.</summary>
    private void DropConnection()
    {
        _connection?.Dispose();
        _connection = null;
    }

    /// <summary>
    /// Begins (or restarts) the background probe for the current selection. Does
    /// nothing while a flash owns the port. Any previously held connection is
    /// dropped first — re-probing establishes a fresh one.
    /// </summary>
    private void RestartProbe()
    {
        CancelProbe();

        // Never drop the connection or reset detection while a flash owns the
        // port — disposing the serial port mid-write is a data race. The flash's
        // finally calls this again with IsBusy already false.
        if (IsBusy)
        {
            IsProbing = false;
            return;
        }

        DropConnection();
        DeviceDetected = false;

        SerialPortInfo? port = SelectedPort;
        if (port is null)
        {
            IsProbing = false;
            if (Ports.Count > 0)
                DeviceInfo = "Select a serial port.";
            return;
        }

        var cts = new CancellationTokenSource();
        _probeCts = cts;
        _ = ProbeLoopAsync(port.PortName, cts.Token);
    }

    /// <summary>
    /// Probes the port repeatedly until a bootloader answers or the selection
    /// changes. Retrying lets the tool pick up a device the instant the user
    /// enters bootloader mode (RESET + blue button) without any extra click.
    /// </summary>
    private async Task ProbeLoopAsync(string portName, CancellationToken token)
    {
        try
        {
            while (!token.IsCancellationRequested)
            {
                Post(() =>
                {
                    IsProbing = true;
                    if (!DeviceDetected)
                        DeviceInfo = $"Identifying {portName}...";
                });

                ProbeResult result = await Task.Run(() => Probe(portName), token).ConfigureAwait(false);
                if (token.IsCancellationRequested)
                {
                    result.Connection?.Dispose();
                    return;
                }

                if (result.Success)
                {
                    // Hold the synced connection open so the flash reuses it.
                    Post(() =>
                    {
                        if (token.IsCancellationRequested)
                        {
                            result.Connection?.Dispose();
                            return;
                        }
                        DropConnection();
                        _connection    = result.Connection;
                        IsProbing      = false;
                        DeviceInfo     = result.Info;
                        DeviceDetected = true;
                    });
                    return;
                }

                Post(() =>
                {
                    IsProbing      = false;
                    DeviceDetected = false;
                    DeviceInfo     = result.Info;
                });

                await Task.Delay(ProbeRetryDelay, token).ConfigureAwait(false);
            }
        }
        catch (OperationCanceledException)
        {
            // Selection changed or shutting down — nothing to do.
        }
    }

    /// <summary>
    /// Opens the port, syncs, and asks the bootloader to identify itself. On
    /// success the open, synced port is handed back inside a <see cref="LiveConnection"/>
    /// (the caller owns and disposes it) so the flash can reuse the same session;
    /// on any failure the port is closed here.
    /// </summary>
    private static ProbeResult Probe(string portName)
    {
        ISerial? serial = null;
        try
        {
            serial = new SerialPortAdapter();
            if (!serial.Open(portName, ProbeConfig))
            {
                serial.Dispose();
                return ProbeResult.Fail($"{portName}: port unavailable (in use?).");
            }

            var client = new An3155Client(serial);
            if (!client.SyncWithRetries(attempts: 2, delayMs: 100))
            {
                serial.Dispose();
                return ProbeResult.Fail($"{portName}: waiting for bootloader — press RESET, then the blue button.");
            }

            GetResult get = client.Get();
            ushort    id  = client.GetId();
            string info =
                $"Connected on {portName}  ·  chip 0x{id:X4}  ·  protocol v{get.ProtocolVersion >> 4}.{get.ProtocolVersion & 0x0F}" +
                $"  ·  {get.SupportedCommands.Length} commands";

            // Keep the port open and synced for the flash (single session).
            var connection = new LiveConnection(serial, client);
            serial = null; // ownership transferred to the connection
            return ProbeResult.Ok(info, connection);
        }
        catch (Exception ex)
        {
            serial?.Dispose();
            return ProbeResult.Fail($"{portName}: {ex.Message}");
        }
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// <summary>Loads a firmware image from a dropped or picked path.</summary>
    public void LoadFirmwareFromPath(string path) => LoadFirmware(path);

    /// <summary>
    /// Loads a firmware image and fills the statistics, or rejects it. Only
    /// <c>.bin</c> files up to <see cref="MaxFirmwareBytes"/> are accepted;
    /// anything else clears the selection and reports why.
    /// </summary>
    private void LoadFirmware(string path)
    {
        if (!string.Equals(Path.GetExtension(path), ".bin", StringComparison.OrdinalIgnoreCase))
        {
            RejectFirmware($"Unsupported file type — choose a .bin image (got {Path.GetFileName(path)}).");
            return;
        }

        try
        {
            long length = new FileInfo(path).Length;
            if (length > MaxFirmwareBytes)
            {
                RejectFirmware($"Image is too large ({length / 1024.0 / 1024.0:F1} MB) — the limit is 2 MB.");
                return;
            }

            var image        = new FirmwareImage(path);
            FirmwareName     = Path.GetFileName(path);
            FirmwareSizeText = $"{image.Size:N0} bytes";
            FirmwareCrcText  = $"0x{image.Crc32:X8}";
            FirmwareError    = null;
            FirmwarePath     = path;   // set last: drives HasFirmware / panels
        }
        catch (Exception ex)
        {
            RejectFirmware($"Cannot load image: {ex.Message}");
        }
    }

    /// <summary>Clears the current image selection and shows why it was rejected.</summary>
    private void RejectFirmware(string reason)
    {
        FirmwareName     = string.Empty;
        FirmwareSizeText = string.Empty;
        FirmwareCrcText  = string.Empty;
        FirmwareError    = reason;
        FirmwarePath     = null;
    }

    /// <summary>Clears the image selection without an error (after a successful flash).</summary>
    private void ClearFirmwareSelection()
    {
        FirmwareName     = string.Empty;
        FirmwareSizeText = string.Empty;
        FirmwareCrcText  = string.Empty;
        FirmwareError    = null;
        FirmwarePath     = null;
    }

    /// <summary>Maps a (stage, current, total) event to the progress bar + caption.</summary>
    private void UpdateProgress(string stage, uint current, uint total)
    {
        Stage         = stage;
        // total == 0 means "no measurable work yet" — show an empty bar, not a
        // misleading full one, until real counts arrive.
        ProgressValue = total == 0 ? 0 : current * 100.0 / total;
        // Only the write phase has a meaningful byte count; erase/verify show just
        // the stage label and bar (no terse "0/1" counter).
        ProgressText  = string.Equals(stage, "Writing", StringComparison.Ordinal)
            ? $"{current / 1024.0:F1} / {total / 1024.0:F1} KB"
            : string.Empty;
    }

    /// <summary>Turns a protocol exception into a plain-language message.</summary>
    private static string Describe(Exception ex) => ex switch
    {
        NackException nack          => $"Device rejected command 0x{nack.Command:X2} (NACK).",
        BootloaderTimeoutException  => $"Timed out waiting for the device: {ex.Message}",
        ConnectionLostException     => $"Connection lost during transfer ({ex.Message}). The bootloader " +
                                       "cannot resume a partial write — re-enter the bootloader (RESET, then " +
                                       "the blue button) and click Retry.",
        ChecksumMismatchException c => $"Verification failed after retries: expected 0x{c.Expected:X8}, device 0x{c.Actual:X8}.",
        _                           => ex.Message,
    };

    /// <summary>Marshals an action onto the UI thread.</summary>
    private static void Post(Action action) => Dispatcher.UIThread.Post(action);

    /// <summary>
    /// Adds a line to the on-screen activity log and mirrors it to the append-only
    /// log file. Must be called on the UI thread (it mutates the bound collection).
    /// </summary>
    private void AppendLog(string level, string message)
    {
        Log.Add(new LogEntry(level, message));
        _sessionLog.Write(level, message);
    }

    /// <summary>Stops the watcher, cancels any in-flight probe/flash, and closes the port.</summary>
    public void Dispose()
    {
        _watcher.PortsChanged -= OnPortsChanged;
        _watcher.Dispose();

        // Ask a running flash to stop before touching the port; the flash owns
        // the connection while busy, so let its finally dispose it rather than
        // pulling the port out from under the writer here.
        _flashCts?.Cancel();
        CancelProbe();
        if (!IsBusy)
            DropConnection();
    }
}
