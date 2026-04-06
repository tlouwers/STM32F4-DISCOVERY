/**
 * \file    CRC.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Crc
 *
 * \brief   Crc peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/CRC
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Crc/CRC.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_crc.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal CRC instance administration.
 */
Crc::Crc() :
    mInitialized(false)
{
    mHandle.Instance = CRC;
}

/**
 * \brief   Destructor.
 */
Crc::~Crc()
{
    Sleep();
}

/**
 * \brief   Initializes the CRC instance.
 * \returns True if the CRC instance could be initialized, else false.
 */
bool Crc::Init()
{
    CheckAndEnableAHBPeripheralClock();

    if (HAL_CRC_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if CRC is initialized.
 * \returns True if CRC is initialized, else false.
 */
bool Crc::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the CRC module in sleep mode.
 * \returns True if CRC module could be put in sleep mode, else false.
 */
bool Crc::Sleep()
{
    mInitialized = false;

    if (HAL_CRC_DeInit(&mHandle) == HAL_OK)
    {
        CheckAndDisableAHBPeripheralClock();
        return true;
    }
    return false;
}

/**
 * \brief   Calculates the CRC32 over the given buffer.
 * \param   buffer  Pointer to the first element in the buffer.
 * \param   length  The length of the buffer to calculate the CRC32 for.
 * \returns The CRC32 if successful, else 0.
 * \note    Asserts if buffer is nullptr or length is 0.
 */
uint32_t Crc::Calculate(uint32_t* buffer, uint32_t length)
{
    EXPECT(buffer);
    EXPECT(length > 0);

    if (buffer == nullptr) { return 0; }
    if (length == 0)       { return 0; }
    if (!mInitialized)     { return 0; }

    return HAL_CRC_Calculate(&mHandle, buffer, length);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Check if the appropriate AHB peripheral clock for the CRC
 *          instance is enabled, if not enable it.
 */
void Crc::CheckAndEnableAHBPeripheralClock()
{
    if (__HAL_RCC_CRC_IS_CLK_DISABLED()) { __HAL_RCC_CRC_CLK_ENABLE(); }
}

/**
 * \brief   Check if the appropriate AHB peripheral clock for the CRC
 *          instance is enabled, if so disable it.
 */
void Crc::CheckAndDisableAHBPeripheralClock()
{
    if (__HAL_RCC_CRC_IS_CLK_ENABLED()) { __HAL_RCC_CRC_CLK_DISABLE(); };
}
