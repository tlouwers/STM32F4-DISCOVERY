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
//  callback backed by the window's StorageProvider (the view owns the TopLevel
//  the dialog needs).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using Avalonia.Controls;
using Avalonia.Platform.Storage;
using BootloaderTool.ViewModels;

namespace BootloaderTool.Views;

/// <summary>The application's main window.</summary>
public partial class MainWindow : Window
{
    /// <summary>Initialises the window from XAML.</summary>
    public MainWindow()
    {
        InitializeComponent();
    }

    /// <summary>Wires the view model's file picker to this window's storage dialog.</summary>
    protected override void OnDataContextChanged(EventArgs e)
    {
        base.OnDataContextChanged(e);
        if (DataContext is MainWindowViewModel viewModel)
            viewModel.FilePicker = PickFirmwareAsync;
    }

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
}
