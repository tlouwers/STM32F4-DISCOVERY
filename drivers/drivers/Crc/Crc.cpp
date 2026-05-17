/**
 * \file    Crc.cpp
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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Crc/Crc.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_crc.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal Crc instance administration.
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
 * \brief   Initializes the Crc instance.
 * \returns True if the Crc instance could be initialized, else false.
 */
bool Crc::Init()
{
    CheckAndEnablePeripheralClock();

    if (HAL_CRC_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if Crc is initialized.
 * \returns True if Crc is initialized, else false.
 */
bool Crc::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the Crc module in sleep mode.
 * \returns True if Crc module could be put in sleep mode, else false.
 */
bool Crc::Sleep()
{
    if (HAL_CRC_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    CheckAndDisablePeripheralClock();
    return true;
}

/**
 * \brief   Calculates the CRC32 over the given buffer.
 * \param   buffer  Pointer to the first element in the buffer.
 * \param   length  The length of the buffer to calculate the CRC32 for.
 * \returns The CRC32 if successful, else 0.
 * \note    Asserts if buffer is nullptr or length is 0.
 */
uint32_t Crc::Calculate(const uint32_t* buffer, uint32_t length)
{
    EXPECT(buffer);
    EXPECT(length > 0);

    if (buffer == nullptr) { return 0; }
    if (length == 0)       { return 0; }
    if (!mInitialized)     { return 0; }

    // HAL_CRC_Calculate's pBuffer parameter is non-const, but the function
    // only reads the buffer (writing the words into CRC->DR). Const-cast at
    // the boundary so callers can pass `const` data without lying to the
    // compiler.
    return HAL_CRC_Calculate(&mHandle, const_cast<uint32_t*>(buffer), length);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Enable the peripheral clock for the Crc instance.
 * \note    The Crc peripheral sits on AHB1 (RCC_AHB1ENR.CRCEN); the
 *          underlying CLK_ENABLE macro is idempotent so no
 *          IS_CLK_DISABLED guard is needed.
 */
void Crc::CheckAndEnablePeripheralClock()
{
    __HAL_RCC_CRC_CLK_ENABLE();
}

/**
 * \brief   Disable the peripheral clock for the Crc instance.
 */
void Crc::CheckAndDisablePeripheralClock()
{
    __HAL_RCC_CRC_CLK_DISABLE();
}
