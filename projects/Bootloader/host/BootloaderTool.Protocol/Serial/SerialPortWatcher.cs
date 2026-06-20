// ----------------------------------------------------------------------------
//  SerialPortWatcher.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Background poll loop that watches for serial ports appearing and disappearing
//  (USB-UART cable plugged / unplugged) and raises an event with the current set
//  plus the delta. Resilient by design: overlapping ticks are skipped and a
//  failed enumeration is treated as "no change", so yanking a cable mid-scan
//  cannot crash or corrupt the watcher.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Serial;

/// <summary>
/// Payload of <see cref="SerialPortWatcher.PortsChanged"/>: the full current
/// port set together with the ports added and removed since the previous scan.
/// </summary>
public sealed class SerialPortsChangedEventArgs : EventArgs
{
    /// <summary>Creates the event payload.</summary>
    public SerialPortsChangedEventArgs(
        IReadOnlyList<SerialPortInfo> ports,
        IReadOnlyList<SerialPortInfo> added,
        IReadOnlyList<SerialPortInfo> removed)
    {
        Ports   = ports;
        Added   = added;
        Removed = removed;
    }

    /// <summary>All ports present after this scan.</summary>
    public IReadOnlyList<SerialPortInfo> Ports { get; }

    /// <summary>Ports that appeared since the previous scan.</summary>
    public IReadOnlyList<SerialPortInfo> Added { get; }

    /// <summary>Ports that disappeared since the previous scan.</summary>
    public IReadOnlyList<SerialPortInfo> Removed { get; }
}

/// <summary>
/// Polls an <see cref="ISerialPortEnumerator"/> on a background timer and raises
/// <see cref="PortsChanged"/> whenever the set of ports changes. The event fires
/// on a thread-pool thread — UI consumers must marshal to their own thread.
/// </summary>
public sealed class SerialPortWatcher : IDisposable
{
    private readonly ISerialPortEnumerator _enumerator;
    private readonly TimeSpan _interval;
    private readonly object _gate = new();

    private Timer? _timer;
    private int _scanning;
    private volatile bool _disposed;
    private IReadOnlyList<SerialPortInfo> _current = Array.Empty<SerialPortInfo>();

    /// <summary>Raised on a thread-pool thread whenever the port set changes.</summary>
    public event EventHandler<SerialPortsChangedEventArgs>? PortsChanged;

    /// <summary>Creates a watcher (call <see cref="Start"/> to begin polling).</summary>
    /// <param name="enumerator">Port source.</param>
    /// <param name="interval">Poll interval; defaults to one second.</param>
    public SerialPortWatcher(ISerialPortEnumerator enumerator, TimeSpan? interval = null)
    {
        _enumerator = enumerator ?? throw new ArgumentNullException(nameof(enumerator));
        _interval   = interval ?? TimeSpan.FromSeconds(1);
    }

    /// <summary>The most recent port set seen by the watcher.</summary>
    public IReadOnlyList<SerialPortInfo> Current
    {
        get { lock (_gate) return _current; }
    }

    /// <summary>Begins polling. The first scan runs almost immediately.</summary>
    public void Start()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(SerialPortWatcher));

        _timer ??= new Timer(_ => Poll(), null, TimeSpan.Zero, _interval);
    }

    /// <summary>
    /// Runs a single scan and raises <see cref="PortsChanged"/> if the set
    /// changed. Exposed for testing and called by the timer. Overlapping calls
    /// return immediately, and a throwing enumerator is treated as "no change".
    /// </summary>
    public void Poll()
    {
        // Skip if a scan is already in flight (slow enumeration, short interval).
        if (Interlocked.Exchange(ref _scanning, 1) == 1)
            return;

        try
        {
            IReadOnlyList<SerialPortInfo> latest;
            try
            {
                latest = _enumerator.Enumerate();
            }
            catch
            {
                // Transient OS failure (often a device mid-removal). Leave the
                // last known set in place and try again next tick.
                return;
            }

            IReadOnlyList<SerialPortInfo> previous;
            lock (_gate)
            {
                previous = _current;
                _current = latest;
            }

            var previousNames = new HashSet<string>(
                previous.Select(p => p.PortName), StringComparer.OrdinalIgnoreCase);
            var latestNames = new HashSet<string>(
                latest.Select(p => p.PortName), StringComparer.OrdinalIgnoreCase);

            List<SerialPortInfo> added   = latest.Where(p => !previousNames.Contains(p.PortName)).ToList();
            List<SerialPortInfo> removed = previous.Where(p => !latestNames.Contains(p.PortName)).ToList();

            // Don't raise after disposal: a scan already in flight when Dispose
            // runs would otherwise deliver an event to a torn-down consumer.
            if ((added.Count > 0 || removed.Count > 0) && !_disposed)
                PortsChanged?.Invoke(this, new SerialPortsChangedEventArgs(latest, added, removed));
        }
        finally
        {
            Interlocked.Exchange(ref _scanning, 0);
        }
    }

    /// <summary>Stops polling and releases the timer.</summary>
    public void Dispose()
    {
        _disposed = true;
        _timer?.Dispose();
        _timer = null;
    }
}
