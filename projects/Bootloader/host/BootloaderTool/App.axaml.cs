// ----------------------------------------------------------------------------
//  App.axaml.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Avalonia Application: loads the XAML styles and creates the main window with
//  its view model on desktop startup.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using BootloaderTool.ViewModels;
using BootloaderTool.Views;

namespace BootloaderTool;

/// <summary>The Avalonia application root.</summary>
public partial class App : Application
{
    /// <summary>Loads the application XAML (styles, theme).</summary>
    public override void Initialize() => AvaloniaXamlLoader.Load(this);

    /// <summary>Creates the main window once the framework is ready.</summary>
    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            desktop.MainWindow = new MainWindow
            {
                DataContext = new MainWindowViewModel(),
            };
        }

        base.OnFrameworkInitializationCompleted();
    }
}
