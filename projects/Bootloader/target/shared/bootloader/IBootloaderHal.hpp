/**
 * \file    IBootloaderHal.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Hardware seam for the bootloader-entry module.
 *
 * \details Abstracts the few register/CPU operations BootloaderEntry needs:
 *          reading and writing the factory-reset magic in the RTC backup
 *          register, performing a system reset, and jumping to the ST system
 *          memory bootloader. Keeping these behind an interface lets the
 *          entry logic be unit tested on the host with a GMock substitute,
 *          while the real implementation (Stm32BootloaderHal) pokes hardware.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/shared/bootloader
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef IBOOTLOADER_HAL_HPP_
#define IBOOTLOADER_HAL_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class IBootloaderHal
{
public:
    virtual ~IBootloaderHal() {}

    /**
     * \brief   Reads the factory-reset magic from the RTC backup register.
     * \returns The current value of the backup register.
     */
    virtual uint32_t ReadMagic() const = 0;

    /**
     * \brief   Writes a value to the RTC backup register, enabling backup
     *          domain access first.
     * \param   value   Value to store (the magic, or 0 to clear).
     */
    virtual void WriteMagic(uint32_t value) = 0;

    /**
     * \brief   Performs a software system reset. Does not return.
     */
    virtual void SystemReset() = 0;

    /**
     * \brief   Hands control to the ST system memory bootloader at
     *          0x1FFF0000. Does not return.
     */
    virtual void JumpToSystemMemory() = 0;
};


#endif  // IBOOTLOADER_HAL_HPP_
