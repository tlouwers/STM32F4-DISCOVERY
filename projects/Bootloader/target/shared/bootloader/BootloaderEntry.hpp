/**
 * \file    BootloaderEntry.hpp
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
 * \details Implements the software path into the ST system memory bootloader.
 *          On startup CheckAndEnterBootloader() inspects a magic value in the
 *          RTC backup register; if present it clears the magic and jumps to
 *          the ST bootloader. TriggerFactoryReset() is called by the running
 *          application to arm that path: it writes the magic and resets, so
 *          the next startup lands in the bootloader. All hardware access goes
 *          through IBootloaderHal so the logic is host-testable.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/shared/bootloader
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef BOOTLOADER_ENTRY_HPP_
#define BOOTLOADER_ENTRY_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>

#include "bootloader/IBootloaderHal.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class BootloaderEntry final
{
public:
    /** Magic value stored in the RTC backup register to request a jump into
     *  the ST system memory bootloader on the next startup. */
    static constexpr uint32_t kFactoryResetMagic = 0xDEADBEEFu;

    explicit BootloaderEntry(IBootloaderHal& hal);

    bool CheckAndEnterBootloader();
    void TriggerFactoryReset();

    // Explicit disabled constructors/operators
    BootloaderEntry(const BootloaderEntry&) = delete;
    BootloaderEntry& operator=(const BootloaderEntry&) = delete;
    BootloaderEntry(BootloaderEntry&&) = delete;
    BootloaderEntry& operator=(BootloaderEntry&&) = delete;

private:
    IBootloaderHal& mHal;
};


#endif  // BOOTLOADER_ENTRY_HPP_
