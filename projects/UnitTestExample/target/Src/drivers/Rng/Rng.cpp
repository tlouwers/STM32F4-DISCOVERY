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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/Rng
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
 * \brief   Constructor, prepares the internal RNG instance administration.
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
 * \brief   Initializes the RNG instance.
 * \returns True if the RNG instance could be initialized, else false.
 */
bool Rng::Init()
{
    CheckAndEnableAHBPeripheralClock();

    if (HAL_RNG_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if RNG is initialized.
 * \returns True if RNG is initialized, else false.
 */
bool Rng::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the RNG module in sleep mode.
 * \returns True if RNG module could be put in sleep mode, else false.
 */
bool Rng::Sleep()
{
    mInitialized = false;

    if (HAL_RNG_DeInit(&mHandle) == HAL_OK)
    {
        CheckAndDisableAHBPeripheralClock();
        return true;
    }
    return false;
}

/**
 * \brief   Get a random number. Call is blocking.
 * \returns A random number if successful, else 0.
 * \note    Asserts when RNG module HAL call did not return succesful.
 */
uint32_t Rng::GetRandom()
{
    if (!mInitialized) { return 0; }

    uint32_t random = 0;

    if (HAL_RNG_GenerateRandomNumber(&mHandle, &random) == HAL_OK)
    {
        return random;
    }

    EXPECT(random != 0);    // HAL gave error, timeout or peripheral still busy
    return 0;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Check if the appropriate AHB peripheral clock for the RNG
 *          instance is enabled, if not enable it.
 */
void Rng::CheckAndEnableAHBPeripheralClock()
{ 
    if (__HAL_RCC_RNG_IS_CLK_DISABLED()) { __HAL_RCC_RNG_CLK_ENABLE(); }
}

/**
 * \brief   Check if the appropriate AHB peripheral clock for the RNG
 *          instance is enabled, if so disable it.
 */
void Rng::CheckAndDisableAHBPeripheralClock()
{
    if (__HAL_RCC_RNG_IS_CLK_ENABLED()) { __HAL_RCC_RNG_CLK_DISABLE(); };
}
