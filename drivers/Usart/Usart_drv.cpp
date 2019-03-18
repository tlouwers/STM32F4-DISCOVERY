/**
 * \file Usart_drv.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   ...
 *
 * \details Intended use is to ...
 *
 * \author      T. Louwers <t.louwers@gmail.com>
 * \version     1.0
 * \date        03-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Usart_drv.hpp"
#include <cassert>


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
static void CheckAndEnableAHB1PeripheralClock(const UsartInstance& usartInstance)
{
    switch (usartInstance)
    {
        case UsartInstance::USART_1: if (__HAL_RCC_USART1_IS_CLK_DISABLED()) { __HAL_RCC_USART1_CLK_ENABLE(); } break;
        case UsartInstance::USART_2: if (__HAL_RCC_USART2_IS_CLK_DISABLED()) { __HAL_RCC_USART2_CLK_ENABLE(); } break;
        case UsartInstance::USART_3: if (__HAL_RCC_USART3_IS_CLK_DISABLED()) { __HAL_RCC_USART3_CLK_ENABLE(); } break;
        case UsartInstance::USART_6: if (__HAL_RCC_USART6_IS_CLK_DISABLED()) { __HAL_RCC_USART6_CLK_ENABLE(); } break;

        default: assert(false); break;      // Invalid usart instance
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
Usart_drv::Usart_drv(const UsartInstance& usartInstance) :
    mUsartInstance(usartInstance)
{

}

Usart_drv::~Usart_drv()
{
    // TODO Auto-generated destructor stub
}

bool Usart_drv::Init(const Config& config)
{
    UART_HandleTypeDef USART_InitStructure = {0};

    USART_InitStructure.Instance = USART2;

    uint32_t parity = UART_PARITY_NONE;
    switch (config.mParity)
    {
        case Parity::EVEN: parity = UART_PARITY_EVEN; break;
        case Parity::ODD:  parity = UART_PARITY_ODD;  break;
        case Parity::NO:   parity = UART_PARITY_NONE; break;
        default: assert(false); break;
    }

    USART_InitStructure.Init.BaudRate     = static_cast<uint32_t>(config.mBaudrate);
    USART_InitStructure.Init.WordLength   = (config.mWordLength == WordLength::_8_BIT) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    USART_InitStructure.Init.Parity       = parity;
    USART_InitStructure.Init.StopBits     = (config.mStopBits == StopBits::_1_BIT) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    USART_InitStructure.Init.Mode         = UART_MODE_TX_RX;
    USART_InitStructure.Init.OverSampling = (config.mOverSampling == OverSampling::_8_TIMES) ? UART_OVERSAMPLING_8 : UART_OVERSAMPLING_16;
    USART_InitStructure.Init.HwFlowCtl    = (config.mUseHardwareFlowControl) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;

    if (HAL_UART_Init(&USART_InitStructure) == HAL_OK)
    {
        return true;
    }
    return false;
}

bool Usart_drv::Sleep() const
{

    return false;
}

//bool Usart_drv::Write(const uint8_t* src, size_t length, const std::function<void()>& refHandler);
//bool Usart_drv::Read(uint8_t* dest, size_t length, const std::function<void()>& refHandler);

bool Usart_drv::WriteBlocking(const uint8_t* src, size_t length)
{

    return false;
}

bool Usart_drv::ReadBlocking(uint8_t* dest, size_t length)
{

    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/

