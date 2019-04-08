/**
 * \file Usart.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   USART peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/Usart
 *
 * \details Intended use is to provide an easier means to work with the USART
 *          peripheral. This class assumes the pins to use for the USART are
 *          already configured.
 *
 *          As example:
 *
 *          // Declare the class (in Application.hpp for example):
 *          Usart mUsart;
 *
 *          // Construct the class, indicate the instance to use:
 *          Application::Application() :
 *              mUsart(UsartInstance::USART_2)
 *          {}
 *
 *          // To Write (interrupt based):
 *          uint8_t write_buffer[] = "test\r\n";
 *          bool result = mUsart.WriteInterrupt(write_buffer, sizeof(write_buffer), [this]() { this->WriteDone(); } );
 *          assert(result);
 *
 *          // To Read (interrupt based):
 *          uint8_t read_buffer[6] = {0};
 *          result = mUsart.ReadInterrupt(read_buffer, sizeof(read_buffer), [this](uint16_t bytesReceived) { this->ReadDone(bytesReceived); });
 *          assert(result);
 *
 *          // The ReadDone callback (as example):
 *          void Application::ReadDone(uint16_t bytesReceived)
 *          {
 *              if (bytesReceived > 0)
 *              {
 *                  // Do stuff ...
 *              }
 *          }
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Usart/Usart.hpp"
#include "utility/SlimAssert.h"
#include "stm32f4xx_hal_usart.h"


/************************************************************************/
/* Alias                                                                */
/************************************************************************/
/**
 * \brief   Constant representing the maximum transmission length of a
 *          single transmission packet.
 */
constexpr uint16_t MAX_TRANSMISSION_LENGTH = UINT16_MAX;


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static UsartCallbacks usart1_callbacks {};
static UsartCallbacks usart2_callbacks {};
static UsartCallbacks usart3_callbacks {};
static UsartCallbacks usart6_callbacks {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   usart_variables     Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const UsartCallbacks& usart_callbacks)
{
    if (usart_callbacks.callbackIRQ)
    {
        usart_callbacks.callbackIRQ();
    }
}

/**
 * \brief   Call the callbackTx, if configured.
 * \param   usart_variables     Structure containing the callbackTx to call.
 */
static void CallbackTxDone(const UsartCallbacks& usart_callbacks)
{
    if (usart_callbacks.callbackTx)
    {
        usart_callbacks.callbackTx();
    }
}

/**
 * \brief   Call the callbackRx, if configured.
 * \param   usart_variables     Structure containing the callbackRx to call.
 * \param   bytesReceived       The actual number of bytes received.
 */
static void CallbackRxDone(const UsartCallbacks& usart_callbacks, uint16_t bytesReceived)
{
    if (usart_callbacks.callbackRx)
    {
        usart_callbacks.callbackRx(bytesReceived);
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal USART instance administration.
 * \param   instance    The USART instance to use.
 */
Usart::Usart(const UsartInstance& instance) :
    mInstance(instance),
    mUsartCallbacks( (instance == UsartInstance::USART_1) ? (usart1_callbacks) : ( (instance == UsartInstance::USART_2) ? (usart2_callbacks) : ( (instance == UsartInstance::USART_3) ? (usart3_callbacks) : (usart6_callbacks) ) ) )
{
    SetInstance(instance);

    mUsartCallbacks.callbackIRQ = [this]() { this->CallbackIRQ(); };
}

/**
 * \brief   Destructor, disabled interrupts.
 */
Usart::~Usart()
{
    // Disable interrupts
    const IRQn_Type irq = GetIRQn(mInstance);
    HAL_NVIC_DisableIRQ(irq);

    mInitialized = false;
}

/**
 * \brief   Initialises the USART instance with the given configuration.
 * \param   config  The configuration for the USART instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool Usart::Init(const Config& config)
{
    CheckAndEnableAHB1PeripheralClock(mInstance);

    uint32_t parity = UART_PARITY_NONE;
    switch (config.mParity)
    {
        case Parity::EVEN: parity = UART_PARITY_EVEN; break;
        case Parity::ODD:  parity = UART_PARITY_ODD;  break;
        case Parity::NO:   parity = UART_PARITY_NONE; break;
        default: ASSERT(false); break;
    }

    mHandle.Init.BaudRate     = static_cast<uint32_t>(config.mBaudrate);
    mHandle.Init.WordLength   = (config.mWordLength == WordLength::_8_BIT) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    mHandle.Init.Parity       = parity;
    mHandle.Init.StopBits     = (config.mStopBits == StopBits::_1_BIT) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    mHandle.Init.Mode         = UART_MODE_TX_RX;
    mHandle.Init.OverSampling = (config.mOverSampling == OverSampling::_8_TIMES) ? UART_OVERSAMPLING_8 : UART_OVERSAMPLING_16;
    mHandle.Init.HwFlowCtl    = (config.mUseHardwareFlowControl) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;

    if (HAL_UART_Init(&mHandle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt
        const IRQn_Type irq = GetIRQn(mInstance);
        HAL_NVIC_DisableIRQ(irq);
        HAL_NVIC_ClearPendingIRQ(irq);
        HAL_NVIC_SetPriority(irq, config.mInterruptPriority, 0);

        __HAL_USART_CLEAR_FLAG(&mHandle, USART_FLAG_IDLE);

        HAL_NVIC_EnableIRQ(irq);

        mInitialized = true;

        return true;
    }
    return false;
}

/**
 * \brief    Puts the Usart module in sleep mode.
 */
void Usart::Sleep()
{
    // Disable interrupts
    const IRQn_Type irq = GetIRQn(mInstance);
    HAL_NVIC_DisableIRQ(irq);

    mInitialized = false;

    // ToDo: low power state, check recovery after sleep
}

/**
 * \brief   Write data using interrupts.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   handler     Callback to call when write completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool Usart::WriteInterrupt(const uint8_t* src, size_t length, const std::function<void()>& handler)
{
    ASSERT(src);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    if (!mInitialized) { return false; }

    mUsartCallbacks.callbackTx = handler;

    // Note: HAL_UART_Transmit_IT will check for src == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Transmit_IT(&mHandle, const_cast<uint8_t*>(src), length))
    {
        return true;
    }
    return false;
}

/**
 * \brief   Read data using interrupts.
 * \param   dest                Pointer to buffer where to store the read data.
 * \param   length              Length of the data to read in bytes.
 * \param   handler             Callback to call when read completed.
 * \param   useIdleDetection    Use IDLE line detection or not, default true.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool Usart::ReadInterrupt(uint8_t* dest, size_t length, const std::function<void(uint16_t)>& handler, bool useIdleDetection /* = true */)
{
    ASSERT(dest);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    if (!mInitialized) { return false; }

    mUsartCallbacks.callbackRx = handler;

    // Note: HAL_UART_Receive will check for dest == nullptr and size == 0 --> returns HAL_ERROR.

    if (useIdleDetection)
    {
        __HAL_USART_ENABLE_IT(&mHandle, USART_IT_IDLE);
    }

    if (HAL_OK == HAL_UART_Receive_IT(&mHandle, dest, length))
    {
        return true;
    }
    return false;
}

/**
 * \brief   Write data using blocking call.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \returns True if the write was successful, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool Usart::WriteBlocking(const uint8_t* src, size_t length)
{
    ASSERT(src);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    if (!mInitialized) { return false; }

    // Note: HAL_UART_Transmit will check for src == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Transmit(&mHandle, const_cast<uint8_t*>(src), length, HAL_MAX_DELAY))
    {
        return true;
    }
    return false;
}

/**
 * \brief   Read data using blocking call.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \returns True if the read was successful, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool Usart::ReadBlocking(uint8_t* dest, size_t length)
{
    ASSERT(dest);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    if (!mInitialized) { return false; }

    // Note: HAL_UART_Receive will check for dest == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Receive(&mHandle, dest, length, HAL_MAX_DELAY))
    {
        return true;
    }
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the USART instance into internal administration.
 * \param   instance    The USART instance to use.
 * \note    Asserts if the USART instance is invalid.
 */
void Usart::SetInstance(const UsartInstance& instance)
{
    switch (instance)
    {
        case UsartInstance::USART_1:
            mHandle.Instance = USART1;
            break;
        case UsartInstance::USART_2:
            mHandle.Instance = USART2;
            break;
        case UsartInstance::USART_3:
            mHandle.Instance = USART3;
            break;
        case UsartInstance::USART_6:
            mHandle.Instance = USART6;
            break;
        default: ASSERT(false); break;      // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the USART
 *          instance is enabled, if not enable it.
 * \param   instance    The USART instance to enable the clock for.
 * \note    Asserts if not a valid USART instance provided.
 */
void Usart::CheckAndEnableAHB1PeripheralClock(const UsartInstance& instance)
{
    switch (instance)
    {
        case UsartInstance::USART_1: if (__HAL_RCC_USART1_IS_CLK_DISABLED()) { __HAL_RCC_USART1_CLK_ENABLE(); } break;
        case UsartInstance::USART_2: if (__HAL_RCC_USART2_IS_CLK_DISABLED()) { __HAL_RCC_USART2_CLK_ENABLE(); } break;
        case UsartInstance::USART_3: if (__HAL_RCC_USART3_IS_CLK_DISABLED()) { __HAL_RCC_USART3_CLK_ENABLE(); } break;
        case UsartInstance::USART_6: if (__HAL_RCC_USART6_IS_CLK_DISABLED()) { __HAL_RCC_USART6_CLK_ENABLE(); } break;
        default: ASSERT(false); break;      // Impossible selection
    }
}

/**
 * \brief   Get the IRQ belonging to the USART.
 * \param   instance    The USART instance to get the IRQ for.
 * \returns The interrupt line IRQ to which the USART belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid USART instance provided.
 */
IRQn_Type Usart::GetIRQn(const UsartInstance& instance)
{
    switch (instance)
    {
        case UsartInstance::USART_1: return USART1_IRQn; break;
        case UsartInstance::USART_2: return USART2_IRQn; break;
        case UsartInstance::USART_3: return USART3_IRQn; break;
        case UsartInstance::USART_6: return USART6_IRQn; break;
        default: ASSERT(false); while(1) { __NOP(); } return USART1_IRQn; break;      // Impossible selection
    }
}

/**
 * \brief
 */
void Usart::CallbackIRQ()
{
    // Check if the 'IDLE' flag is set, if so call end of Rx callback, the clear flag.
    if (__HAL_USART_GET_FLAG(&mHandle, USART_FLAG_IDLE))
    {
        HAL_UART_RxCpltCallback(&mHandle);

        // End the transmission, received line IDLE - clears RxState
        HAL_UART_AbortReceive_IT(&mHandle);
    }

    // If it was another interrupt, pass it through
    HAL_UART_IRQHandler(&mHandle);
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the USART TX completed interrupt into a TX
 *          callback.
 * \param   handle  The USART handle from which the TX ISR came.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    if (handle->Instance == USART1) { CallbackTxDone(usart1_callbacks); }
    if (handle->Instance == USART2) { CallbackTxDone(usart2_callbacks); }
    if (handle->Instance == USART3) { CallbackTxDone(usart3_callbacks); }
    if (handle->Instance == USART6) { CallbackTxDone(usart6_callbacks); }
}

/**
 * \brief   ISR: handler to dispatch the USART RX completed interrupt into a RX
 *          callback.
 * \param   handle  The USART handle from which the RX ISR came.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    // Disable and clear IDLE line interrupt
    // See reference manual, USART section, IDLE line detected interrupt
    __HAL_USART_DISABLE_IT(handle, USART_IT_IDLE);
    __HAL_USART_CLEAR_FLAG(handle, USART_FLAG_IDLE);
    uint32_t dummy = handle->Instance->DR;
    (void)(dummy);

    // Check to see if data is actually received: when receiving data normally, the IDLE line
    // interrupt will always fire after the regular Rx interrupt.  by checking the RxXferSize
    // and clearing it after calling the user callback we prevent calling the user callback
    // when no data was received (ignores the IDLE line interrupt afterwards).
    if (handle->RxXferSize > 0)
    {
        uint16_t bytesReceived = handle->RxXferSize - handle->RxXferCount;

        if (handle->Instance == USART1) { CallbackRxDone(usart1_callbacks, bytesReceived); }
        if (handle->Instance == USART2) { CallbackRxDone(usart2_callbacks, bytesReceived); }
        if (handle->Instance == USART3) { CallbackRxDone(usart3_callbacks, bytesReceived); }
        if (handle->Instance == USART6) { CallbackRxDone(usart6_callbacks, bytesReceived); }
    }

    handle->RxXferSize = 0;
}

/**
 * \brief   ISR: route USART1 interrupts to 'CallbackIRQ'.
 */
extern "C" void USART1_IRQHandler(void)
{
    CallbackIRQ(usart1_callbacks);
}

/**
 * \brief   ISR: route USART2 interrupts to 'CallbackIRQ'.
 */
extern "C" void USART2_IRQHandler(void)
{
    CallbackIRQ(usart2_callbacks);
}

/**
 * \brief   ISR: route USART3 interrupts to 'CallbackIRQ'.
 */
extern "C" void USART3_IRQHandler(void)
{
    CallbackIRQ(usart3_callbacks);
}

/**
 * \brief   ISR: route USART6 interrupts to 'CallbackIRQ'.
 */
extern "C" void USART6_IRQHandler(void)
{
    CallbackIRQ(usart6_callbacks);
}