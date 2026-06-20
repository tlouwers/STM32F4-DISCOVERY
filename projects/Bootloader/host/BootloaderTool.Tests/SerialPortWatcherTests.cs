// ----------------------------------------------------------------------------
//  SerialPortWatcherTests.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  xUnit tests for SerialPortWatcher: the add/remove delta, no-change quiet, and
//  resilience to an enumerator that throws (cable yanked mid-scan). Driven via
//  Poll() with a scripted enumerator — no timer, no real hardware.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using BootloaderTool.Protocol.Serial;
using Xunit;

namespace BootloaderTool.Tests;

public class SerialPortWatcherTests
{
    /// <summary>Scriptable enumerator: set the current ports, or make it throw.</summary>
    private sealed class FakeEnumerator : ISerialPortEnumerator
    {
        public List<SerialPortInfo> Current { get; } = new();
        public bool Throw { get; set; }
        public int Calls { get; private set; }

        public void Set(params string[] names)
        {
            Current.Clear();
            foreach (string name in names)
                Current.Add(new SerialPortInfo(name, null));
        }

        public IReadOnlyList<SerialPortInfo> Enumerate()
        {
            Calls++;
            if (Throw)
                throw new InvalidOperationException("enumeration failed");
            return Current.ToList();
        }
    }

    private static SerialPortsChangedEventArgs? CaptureSingle(FakeEnumerator fake, Action<SerialPortWatcher> act)
    {
        using var watcher = new SerialPortWatcher(fake);
        SerialPortsChangedEventArgs? captured = null;
        watcher.PortsChanged += (_, e) => captured = e;
        act(watcher);
        return captured;
    }

    [Fact]
    public void Poll_FirstScan_RaisesAddedForAllPorts()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1", "COM3");

        SerialPortsChangedEventArgs? e = CaptureSingle(fake, w => w.Poll());

        Assert.NotNull(e);
        Assert.Equal(new[] { "COM1", "COM3" }, e!.Added.Select(p => p.PortName));
        Assert.Empty(e.Removed);
        Assert.Equal(2, e.Ports.Count);
    }

    [Fact]
    public void Poll_PortAdded_RaisesOnlyTheNewPort()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1");

        using var watcher = new SerialPortWatcher(fake);
        var events = new List<SerialPortsChangedEventArgs>();
        watcher.PortsChanged += (_, e) => events.Add(e);

        watcher.Poll();              // initial: COM1 added
        fake.Set("COM1", "COM5");
        watcher.Poll();              // COM5 added

        Assert.Equal(2, events.Count);
        Assert.Equal(new[] { "COM5" }, events[1].Added.Select(p => p.PortName));
        Assert.Empty(events[1].Removed);
    }

    [Fact]
    public void Poll_PortRemoved_RaisesRemoved()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1", "COM5");

        using var watcher = new SerialPortWatcher(fake);
        var events = new List<SerialPortsChangedEventArgs>();
        watcher.PortsChanged += (_, e) => events.Add(e);

        watcher.Poll();              // initial
        fake.Set("COM1");            // COM5 unplugged
        watcher.Poll();

        Assert.Equal(new[] { "COM5" }, events[1].Removed.Select(p => p.PortName));
        Assert.Empty(events[1].Added);
    }

    [Fact]
    public void Poll_NoChange_DoesNotRaise()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1");

        using var watcher = new SerialPortWatcher(fake);
        int raised = 0;
        watcher.PortsChanged += (_, _) => raised++;

        watcher.Poll();              // initial — raises once
        watcher.Poll();              // identical — quiet
        watcher.Poll();              // identical — quiet

        Assert.Equal(1, raised);
    }

    [Fact]
    public void Poll_EnumeratorThrows_DoesNotRaiseAndKeepsLastState()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1");

        using var watcher = new SerialPortWatcher(fake);
        int raised = 0;
        watcher.PortsChanged += (_, _) => raised++;

        watcher.Poll();              // COM1 added
        fake.Throw = true;
        watcher.Poll();              // throws internally — swallowed

        Assert.Equal(1, raised);
        Assert.Equal(new[] { "COM1" }, watcher.Current.Select(p => p.PortName));
    }

    [Fact]
    public void Poll_RecoversAfterTransientFailure()
    {
        var fake = new FakeEnumerator();
        fake.Set("COM1");

        using var watcher = new SerialPortWatcher(fake);
        var events = new List<SerialPortsChangedEventArgs>();
        watcher.PortsChanged += (_, e) => events.Add(e);

        watcher.Poll();              // COM1 added
        fake.Throw = true;
        watcher.Poll();              // swallowed
        fake.Throw = false;
        fake.Set("COM1", "COM7");
        watcher.Poll();              // COM7 added — diff is against the kept state

        Assert.Equal(2, events.Count);
        Assert.Equal(new[] { "COM7" }, events[1].Added.Select(p => p.PortName));
    }

    [Fact]
    public void Dispose_PreventsStart()
    {
        var fake = new FakeEnumerator();
        var watcher = new SerialPortWatcher(fake);
        watcher.Dispose();

        Assert.Throws<ObjectDisposedException>(() => watcher.Start());
    }
}
