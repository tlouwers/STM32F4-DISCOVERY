/**
 * \file    Watchdog.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Watchdog
 *
 * \brief   Watchdog (IWDG) peripheral driver class.
 *
 * \note    The IWDG depends on the LSI clock to be available and running.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/Watchdog
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2022
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Watchdog/Watchdog.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_iwdg.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Initializes the Watchdog instance with the given configuration.
 * \param   config  The configuration for the Watchdog to use.
 * \returns True if the configuration could be applied, else false.
 */
bool Watchdog::Init(const IConfig& config)
{
    // Check if LSI clock is running, if not: assert and fail
    const bool isLSIEnabled = IsLSIClockEnabled();

    ASSERT(isLSIEnabled);

    if (!isLSIEnabled) { return false; }

    // Optional: freeze the independent watchdog timer while debugging
    __HAL_DBGMCU_FREEZE_IWDG();

    const Config& cfg = reinterpret_cast<const Config&>(config);

    mHandle.Init.Prescaler = CalculatePrescaler(cfg.mTimeout);
    mHandle.Init.Reload    = CalculateReload(cfg.mTimeout);

    mHandle.Instance = IWDG;

    if (HAL_IWDG_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if Watchdog is initialized.
 * \returns True if Watchdog is initialized, else false.
 */
bool Watchdog::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Dummy, always returns false.
 * \returns False.
 */
bool Watchdog::Sleep()
{
    return false;
}

/**
 * \brief   Refresh the Watchdog count, prevent expire and reset the board.
 * \details Should be called 'often', but at least within the specified
 *          timeout.
 */
void Watchdog::Refresh() const
{
    HAL_IWDG_Refresh(const_cast<IWDG_HandleTypeDef*>(&mHandle));
};


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Check if the LSI clock is enabled.
 * \details The Watchdog depends on the running 32 kHz LSI clock.
 * \returns True if it is enabled, else false.
 */
bool Watchdog::IsLSIClockEnabled() const
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {};

    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

    return (RCC_OscInitStruct.LSIState == RCC_LSI_ON);
}

/**
 * \brief   Calculate the Watchdog prescale value.
 * \param   timeout     The desired timeout to use.
 * \returns Prescaler value (lowest possible) which matches the given timeout.
 * \note    Works in conjunction with the reload value.
 */
uint32_t Watchdog::CalculatePrescaler(Timeout timeout)
{
    uint32_t prescaler = 0;

    switch (timeout)
    {
        case Timeout::_5_MS:   prescaler = 0; break;
        case Timeout::_10_MS:  prescaler = 0; break;
        case Timeout::_25_MS:  prescaler = 0; break;
        case Timeout::_50_MS:  prescaler = 0; break;
        case Timeout::_125_MS: prescaler = 0; break;
        case Timeout::_250_MS: prescaler = 0; break;
        case Timeout::_500_MS: prescaler = 0; break;
        case Timeout::_1_S:    prescaler = 1; break;
        case Timeout::_2_S:    prescaler = 2; break;
        case Timeout::_4_S:    prescaler = 3; break;
        case Timeout::_8_S:    prescaler = 4; break;
        case Timeout::_16_S:   prescaler = 5; break;
        case Timeout::_32_S:   prescaler = 6; break;
        default: ASSERT(false); break;
    }

    return prescaler;
}

/**
 * \brief   Calculate the Watchdog reload value.
 * \param   timeout     The desired timeout to use.
 * \returns Reload value which matches the given timeout.
 * \note    Works in conjunction with the prescale value.
 */
uint32_t Watchdog::CalculateReload(Timeout timeout)
{
    uint32_t reload = 0;

    switch (timeout)
    {
        case Timeout::_5_MS:   reload = 39;   break;
        case Timeout::_10_MS:  reload = 79;   break;
        case Timeout::_25_MS:  reload = 199;  break;
        case Timeout::_50_MS:  reload = 399;  break;
        case Timeout::_125_MS: reload = 999;  break;
        case Timeout::_250_MS: reload = 1999; break;
        case Timeout::_500_MS: reload = 3999; break;
        case Timeout::_1_S:    reload = 3999; break;
        case Timeout::_2_S:    reload = 3999; break;
        case Timeout::_4_S:    reload = 3999; break;
        case Timeout::_8_S:    reload = 3999; break;
        case Timeout::_16_S:   reload = 3999; break;
        case Timeout::_32_S:   reload = 3999; break;
        default: ASSERT(false); break;
    }

    return reload;
}
