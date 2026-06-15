// ----------------------------------------------------------------------------
//  ViewModelBase.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Base class for view models. Inherits CommunityToolkit.Mvvm change
//  notification so derived types can use [ObservableProperty] / [RelayCommand].
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

using CommunityToolkit.Mvvm.ComponentModel;

namespace BootloaderTool.ViewModels;

/// <summary>Common base for view models (INotifyPropertyChanged via the toolkit).</summary>
public abstract class ViewModelBase : ObservableObject
{
}
