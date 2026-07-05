/**
 * \file    Stm32BootloaderHal.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Stm32BootloaderHal
 *
 * \brief   STM32F4 implementation of IBootloaderHal.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/shared/bootloader
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "bootloader/Stm32BootloaderHal.hpp"

#include "stm32f4xx_hal.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructs the STM32F4 bootloader HAL.
 */
Stm32BootloaderHal::Stm32BootloaderHal()
{
}

/**
 * \brief   Destructor.
 */
Stm32BootloaderHal::~Stm32BootloaderHal()
{
}

/**
 * \brief   Reads RTC backup register 0.
 * \details Enables the PWR clock and backup-domain access first; the backup
 *          registers retain their value across a system reset.
 * \returns The stored 32-bit value.
 */
uint32_t Stm32BootloaderHal::ReadMagic() const
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    return RTC->BKP0R;
}

/**
 * \brief   Writes RTC backup register 0.
 * \param   value   Value to store (the magic, or 0 to clear).
 */
void Stm32BootloaderHal::WriteMagic(uint32_t value)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    RTC->BKP0R = value;
}

/**
 * \brief   Issues a software system reset. Does not return.
 */
void Stm32BootloaderHal::SystemReset()
{
    HAL_NVIC_SystemReset();
}

/**
 * \brief   Deinitialises peripherals, remaps system memory to 0x00000000, and
 *          jumps to the ST system memory bootloader. Does not return.
 *
 * \details Interrupts are disabled and peripherals are torn down via
 *          HAL_DeInit(). HAL_RCC_DeInit() is called too, but it is a
 *          non-functional `__weak` stub in the vendored v1.28.3 HAL (it
 *          `return HAL_OK;` without touching a register) — SYSCLK is left
 *          exactly as Board::InitClock() configured it (8 MHz direct off
 *          HSE, no PLL). Do NOT "fix" this by resetting the clock to HSI or
 *          any other frequency: hardware-tested 2026-07-05 (STM32F407G-DISC1,
 *          COM7) — switching SYSCLK to HSI @ 16 MHz before the jump made the
 *          ST ROM bootloader stop responding to sync entirely (even the
 *          tolerant `info` probe got nothing); reverting to this exact no-op
 *          restored it immediately. The ROM bootloader's autobaud has only
 *          ever been validated jumping from this project's 8 MHz HSE-direct
 *          clock — changing the pre-jump SYSCLK frequency is not a safe
 *          cleanup here, even though it looks like an obvious fix. The
 *          bootloader's initial stack pointer and reset vector are taken
 *          from the first two words at 0x1FFF0000.
 */
void Stm32BootloaderHal::JumpToSystemMemory()
{
    __disable_irq();

    HAL_RCC_DeInit();   // no-op in this HAL version — see \details above, do not "fix"
    HAL_DeInit();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    const uint32_t* sysMem = reinterpret_cast<const uint32_t*>(kSystemMemoryBase);
    __set_MSP(sysMem[0]);
    reinterpret_cast<void (*)()>(sysMem[1])();

    // Never reached.
    while (1)
    {
    }
}
