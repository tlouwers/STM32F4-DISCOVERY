/**
 * \file    BootloaderEntry.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   BootloaderEntry
 *
 * \brief   Bootloader-entry module for the application firmware.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/shared/bootloader
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "bootloader/BootloaderEntry.hpp"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructs the entry module.
 * \param   hal   Hardware seam providing backup-register, reset, and jump
 *                operations.
 */
BootloaderEntry::BootloaderEntry(IBootloaderHal& hal) :
    mHal(hal)
{
}

/**
 * \brief   Checks the RTC backup register for the factory-reset magic and, if
 *          present, hands control to the ST system memory bootloader.
 *
 * \details Must run before any peripheral initialisation. The magic is cleared
 *          before jumping so a subsequent reset boots the application normally
 *          rather than looping back into the bootloader.
 *
 * \returns True if the magic was present and the jump was taken (on real
 *          hardware the jump does not return; the return value supports
 *          host testing). False if the application should boot normally.
 */
bool BootloaderEntry::CheckAndEnterBootloader()
{
    if (kFactoryResetMagic == mHal.ReadMagic())
    {
        mHal.WriteMagic(0);            // clear so the next reset boots the app
        mHal.JumpToSystemMemory();     // does not return on hardware
        return true;
    }
    return false;
}

/**
 * \brief   Arms the bootloader-entry path and resets the device.
 *
 * \details Writes the magic to the RTC backup register and issues a system
 *          reset. After the reset CheckAndEnterBootloader() detects the magic
 *          and jumps into the ST bootloader. Does not return on hardware.
 */
void BootloaderEntry::TriggerFactoryReset()
{
    mHal.WriteMagic(kFactoryResetMagic);
    mHal.SystemReset();                // does not return on hardware
}
