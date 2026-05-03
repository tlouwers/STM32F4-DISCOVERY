/**
 * \file    Rng.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Rng
 *
 * \brief   Hardware random number generator class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Rng
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Rng/Rng.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_rng.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal Rng instance administration.
 */
Rng::Rng() :
    mInitialized(false)
{
    mHandle.Instance = RNG;
}

/**
 * \brief   Destructor.
 */
Rng::~Rng()
{
    Sleep();
}

/**
 * \brief   Initializes the Rng instance.
 * \returns True if the Rng instance could be initialized, else false.
 * \note    RNG_CLK is fed from the dedicated 48 MHz PLL output (PLL48CK,
 *          shared with USB-OTG-FS and SDIO). Init verifies the main PLL
 *          is running and PLLQ is in valid range; without that, the
 *          RNG peripheral has no clock and every GetRandom() would
 *          silently time out.
 */
bool Rng::Init()
{
    if (!IsRngClockConfigured()) { return false; }

    CheckAndEnablePeripheralClock();

    if (HAL_RNG_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if Rng is initialized.
 * \returns True if Rng is initialized, else false.
 */
bool Rng::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the Rng module in sleep mode.
 * \returns True if Rng module could be put in sleep mode, else false.
 */
bool Rng::Sleep()
{
    if (HAL_RNG_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    CheckAndDisablePeripheralClock();
    return true;
}

/**
 * \brief   Get a random number.
 * \param   out     Receives the generated random number on success;
 *                  unchanged on failure.
 * \returns True if a random number was generated, else false.
 * \note    Blocking call -- the HAL polls RNG_SR.DRDY for up to ~40 ms
 *          before returning HAL_TIMEOUT.
 * \note    Soft-asserts when the HAL returned a non-OK status (clock
 *          error, seed error, timeout, or peripheral busy).
 */
bool Rng::GetRandom(uint32_t& out)
{
    if (!mInitialized) { return false; }

    uint32_t random = 0;

    if (HAL_RNG_GenerateRandomNumber(&mHandle, &random) == HAL_OK)
    {
        out = random;
        return true;
    }

    EXPECT(false);    // HAL gave error, timeout or peripheral still busy
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Enable the peripheral clock for the Rng instance.
 * \note    The Rng peripheral sits on AHB2 (RCC_AHB2ENR.RNGEN); the
 *          underlying CLK_ENABLE macro is idempotent so no
 *          IS_CLK_DISABLED guard is needed.
 */
void Rng::CheckAndEnablePeripheralClock()
{
    __HAL_RCC_RNG_CLK_ENABLE();
}

/**
 * \brief   Disable the peripheral clock for the Rng instance.
 */
void Rng::CheckAndDisablePeripheralClock()
{
    __HAL_RCC_RNG_CLK_DISABLE();
}

/**
 * \brief   Check that the 48 MHz PLL output (PLL48CK) feeding RNG_CLK is
 *          configured.
 * \details RNG_CLK is derived from the main PLL's Q divider output. If the
 *          PLL is not running, or PLLQ is out of valid range (RM0090
 *          §6.3.2 Table 26: 2..15), RNG has no clock source --
 *          HAL_RNG_GenerateRandomNumber would time out on every call and
 *          the driver would silently return 0.
 * \returns True if PLL48CK is configured and ready to feed RNG, else false.
 */
bool Rng::IsRngClockConfigured()
{
    if ((RCC->CR & RCC_CR_PLLRDY) == 0U) { return false; }

    const uint32_t pllQ = (RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos;
    return (pllQ >= 2U) && (pllQ <= 15U);
}
