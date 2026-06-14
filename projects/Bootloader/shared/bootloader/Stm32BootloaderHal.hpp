/**
 * \file    Stm32BootloaderHal.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   STM32F4 implementation of IBootloaderHal.
 *
 * \details Concrete hardware seam used in the firmware build. Reads/writes the
 *          factory-reset magic in RTC backup register 0, performs the software
 *          system reset, and executes the deinit + remap + jump sequence into
 *          the ST system memory bootloader at 0x1FFF0000. Not compiled into
 *          the host unit tests, which substitute a mock.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/shared/bootloader
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef STM32_BOOTLOADER_HAL_HPP_
#define STM32_BOOTLOADER_HAL_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>

#include "bootloader/IBootloaderHal.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Stm32BootloaderHal final : public IBootloaderHal
{
public:
    Stm32BootloaderHal();
    virtual ~Stm32BootloaderHal();

    uint32_t ReadMagic() const override;
    void WriteMagic(uint32_t value) override;
    void SystemReset() override;
    void JumpToSystemMemory() override;

private:
    static constexpr uint32_t kSystemMemoryBase = 0x1FFF0000u;
};


#endif  // STM32_BOOTLOADER_HAL_HPP_
