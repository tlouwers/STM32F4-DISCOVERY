// ----------------------------------------------------------------------------
//  MainWindow.axaml.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Code-behind for the main window. Supplies the view model with a file-picker
//  callback backed by the window's StorageProvider, drives the custom window
//  chrome (drag-to-move title bar, minimise / maximise / close buttons), and
//  accepts a dragged-and-dropped firmware image onto the drop zone.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using System.Collections.Specialized;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Platform.Storage;
using Avalonia.Threading;
using BootloaderTool.ViewModels;

namespace BootloaderTool.Views;

/// <summary>The application's main window.</summary>
public partial class MainWindow : Window
{
    /// <summary>Initialises the window from XAML and wires the chrome / drop zone.</summary>
    public MainWindow()
    {
        InitializeComponent();

        TitleBar.PointerPressed += OnTitleBarPointerPressed;
        DropZone.AddHandler(DragDrop.DragOverEvent, OnDropZoneDragOver);
        DropZone.AddHandler(DragDrop.DropEvent, OnDropZoneDrop);
    }

    /// <summary>Stops the port watcher and probe loop when the window closes.</summary>
    protected override void OnClosed(EventArgs e)
    {
        base.OnClosed(e);
        if (DataContext is IDisposable disposable)
            disposable.Dispose();
    }

    /// <summary>The log collection currently subscribed to for auto-scroll.</summary>
    private INotifyCollectionChanged? _subscribedLog;

    /// <summary>Wires the file picker and keeps the activity log scrolled to the latest line.</summary>
    protected override void OnDataContextChanged(EventArgs e)
    {
        base.OnDataContextChanged(e);

        if (_subscribedLog is not null)
        {
            _subscribedLog.CollectionChanged -= OnLogChanged;
            _subscribedLog = null;
        }

        if (DataContext is MainWindowViewModel viewModel)
        {
            viewModel.FilePicker = PickFirmwareAsync;
            viewModel.Log.CollectionChanged += OnLogChanged;
            _subscribedLog = viewModel.Log;
        }
    }

    /// <summary>Scrolls the log to the newest entry whenever a line is added.</summary>
    private void OnLogChanged(object? sender, NotifyCollectionChangedEventArgs e)
        => Dispatcher.UIThread.Post(() => LogScroller.ScrollToEnd(), DispatcherPriority.Background);

    /// <summary>Shows a single-file picker filtered to firmware binaries.</summary>
    /// <returns>The chosen local path, or null if cancelled.</returns>
    private async Task<string?> PickFirmwareAsync()
    {
        IReadOnlyList<IStorageFile> files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title         = "Select firmware image",
            AllowMultiple = false,
            FileTypeFilter = new[]
            {
                new FilePickerFileType("Firmware image (*.bin)") { Patterns = new[] { "*.bin" } },
                FilePickerFileTypes.All,
            },
        });

        return files.Count > 0 ? files[0].TryGetLocalPath() : null;
    }

    /// <summary>Folds the activity sheet back when the dimming scrim is tapped.</summary>
    private void OnScrimTapped(object? sender, TappedEventArgs e)
    {
        if (DataContext is MainWindowViewModel viewModel)
            viewModel.ActivityExpanded = false;
    }

    /************************************************************************/
    /* Window chrome                                                        */
    /************************************************************************/

    /// <summary>Drags the window when the title bar is pressed with the left button.</summary>
    private void OnTitleBarPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
            BeginMoveDrag(e);
    }

    /// <summary>Minimises the window.</summary>
    private void OnMinimize(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
        => WindowState = WindowState.Minimized;

    /// <summary>Toggles between normal and maximised.</summary>
    private void OnMaximize(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
        => WindowState = WindowState == WindowState.Maximized
            ? WindowState.Normal
            : WindowState.Maximized;

    /// <summary>Closes the window.</summary>
    private void OnClose(object? sender, Avalonia.Interactivity.RoutedEventArgs e)
        => Close();

    /************************************************************************/
    /* Drag-and-drop firmware                                               */
    /************************************************************************/

    /// <summary>Allows the drop only when the payload carries a file.</summary>
    private void OnDropZoneDragOver(object? sender, DragEventArgs e)
    {
        e.DragEffects = e.DataTransfer is not null && e.DataTransfer.Contains(DataFormat.File)
            ? DragDropEffects.Copy
            : DragDropEffects.None;
    }

    /// <summary>Loads the dropped local file as the firmware image.</summary>
    private void OnDropZoneDrop(object? sender, DragEventArgs e)
    {
        if (DataContext is not MainWindowViewModel viewModel || e.DataTransfer is null)
            return;

        IStorageItem? item = e.DataTransfer.TryGetFile();
        string? path = item?.TryGetLocalPath();
        if (!string.IsNullOrEmpty(path))
            viewModel.LoadFirmwareFromPath(path);
    }
}
