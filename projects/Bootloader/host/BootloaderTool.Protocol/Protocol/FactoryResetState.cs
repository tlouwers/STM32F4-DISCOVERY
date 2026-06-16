// ----------------------------------------------------------------------------
//  FactoryResetState.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  States of the factory reset orchestrator state machine.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    06-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// States of the <see cref="FactoryResetSession"/> state machine.
/// Normal progression is
/// Idle → Connecting → Erasing → Writing → Verifying → Booting → Done.
/// Any unrecoverable failure transitions to Failed.
/// </summary>
public enum FactoryResetState
{
    /// <summary>Not started.</summary>
    Idle,

    /// <summary>Syncing with the bootloader on initial connect.</summary>
    Connecting,

    /// <summary>Erasing the target flash sectors.</summary>
    Erasing,

    /// <summary>Writing the firmware image in 256-byte chunks.</summary>
    Writing,

    /// <summary>Verifying the written image via the device CRC.</summary>
    Verifying,

    /// <summary>Issuing the Go command to start the application.</summary>
    Booting,

    /// <summary>Completed successfully.</summary>
    Done,

    /// <summary>Aborted on an unrecoverable error.</summary>
    Failed,
}
