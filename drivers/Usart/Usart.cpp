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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/Usart
 *
 * \details Intended use is to ...
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        03-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Usart/Usart.hpp"
#include "utility/SlimAssert.h"


/************************************************************************/
/* Alias                                                                */
/************************************************************************/
constexpr uint16_t MAX_TRANSMISSION_LENGTH = UINT16_MAX;


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    UsartTransmissionType
 * \brief   Usart transmission type.
 */
enum class UsartTransmissionType : bool
{
    Tx,
    Rx
};


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
/**
 * \brief   Internal administration to keep track of the USART handles.
 */
static UART_HandleTypeDef* handleList[4] = {};

/**
 * \brief   Internal administration to keep track of the USART callbacks.
 */
static std::function<void()> callbackList[9] = {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Get the USART handle based upon the USART instance.
 * \param   instance    The USART instance to get the handle for.
 * \returns The handle for the given USART instance, or nullptr if not found.
 * \note    Asserts if not a valid USART instance provided.
 */
UART_HandleTypeDef* GetUsartHandle(USART_TypeDef* instance)
{
    UART_HandleTypeDef* handle = nullptr;

    if (instance != nullptr)
    {
             if (instance == USART1) { handle = handleList[0]; }
        else if (instance == USART2) { handle = handleList[1]; }
        else if (instance == USART3) { handle = handleList[2]; }
        else if (instance == USART6) { handle = handleList[3]; }
        else { ASSERT(false); }     // Impossible selection
    }

    return handle;
}

/**
 * \brief   Get the index to use to find the registered callback.
 * \param   handle  The USART handle to get the callback for.
 * \param   type    The transmission type to get the callback for.
 * \returns An index where the requested callback to call is located,
 *          if invalid an index pointing to the last element of the
 *          callbackList.
 */
static int GetIndex(UART_HandleTypeDef* handle, UsartTransmissionType type)
{
    // If invalid, return an index to the element of the callbackList, which
    // is a nullptr to prevent a spurious callback to trigger.
    int index = sizeof(callbackList) / sizeof(callbackList[0]);

    if (handle != nullptr)
    {
             if (handle->Instance == USART1) { (type == UsartTransmissionType::Tx) ? index = 0 : index = 1; }
        else if (handle->Instance == USART2) { (type == UsartTransmissionType::Tx) ? index = 2 : index = 3; }
        else if (handle->Instance == USART3) { (type == UsartTransmissionType::Tx) ? index = 4 : index = 5; }
        else if (handle->Instance == USART6) { (type == UsartTransmissionType::Tx) ? index = 6 : index = 7; }
        else { ASSERT(false); }     // Invalid instance
    }

    return index;
}

/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the USART
 *          instance is enabled, if not enable it.
 * \param   instance    The USART instance to enable the clock for.
 * \note    Asserts if not a valid USART instance provided.
 */
static void CheckAndEnableAHB1PeripheralClock(USART_TypeDef* instance)
{
         if ((instance == USART1) && __HAL_RCC_USART1_IS_CLK_DISABLED()) { __HAL_RCC_USART1_CLK_ENABLE(); }
    else if ((instance == USART2) && __HAL_RCC_USART2_IS_CLK_DISABLED()) { __HAL_RCC_USART2_CLK_ENABLE(); }
    else if ((instance == USART3) && __HAL_RCC_USART3_IS_CLK_DISABLED()) { __HAL_RCC_USART3_CLK_ENABLE(); }
    else if ((instance == USART6) && __HAL_RCC_USART6_IS_CLK_DISABLED()) { __HAL_RCC_USART6_CLK_ENABLE(); }
    else { ASSERT(false); }     // Invalid instance
}

/**
 * \brief   Get the IRQ belonging to the USART.
 * \param   instance    The USART instance to get the IRQ for.
 * \returns The interrupt line IRQ to which the USART belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid USART instance provided.
 */
static IRQn_Type GetIRQn(USART_TypeDef* instance)
{
         if (instance == USART1) { return USART1_IRQn; }
    else if (instance == USART2) { return USART2_IRQn; }
    else if (instance == USART3) { return USART3_IRQn; }
    else if (instance == USART6) { return USART6_IRQn; }
    else { ASSERT(false); while(1) { __NOP(); } return USART1_IRQn; }   // Invalid instance
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal USART instance administration.
 * \param   instance    The USART instance to use.
 * \note    Asserts if no memory for the internal administration could be
 *          allocated on the heap.
 */
Usart::Usart(const UsartInstance& instance) :
    mInstance(nullptr),
    mInitialized(false)
{
    bool result = SetInstance(instance);
    ASSERT(result);
    (void)(result);
}

/**
 * \brief   Destructor, disabled interrupts, cleans the internal USART instance
 *          administration.
 */
Usart::~Usart()
{
    // Disable interrupts
    const IRQn_Type irq = GetIRQn(mInstance);
    HAL_NVIC_DisableIRQ(irq);

    // Cleanup the administration - only for the given instance
    UART_HandleTypeDef* handle = GetUsartHandle(mInstance);
    for (auto&& i : handleList)
    {
        if (i == handle) { delete i; i = nullptr; }
    }
}

/**
 * \brief   Initialises the USART instance with the given configuration.
 * \param   config  The configuration for the USART instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool Usart::Init(const Config& config)
{
    UART_HandleTypeDef* handle = GetUsartHandle(mInstance);
    
    if (handle == nullptr) { return false; }

    CheckAndEnableAHB1PeripheralClock(mInstance);

    uint32_t parity = UART_PARITY_NONE;
    switch (config.mParity)
    {
        case Parity::EVEN: parity = UART_PARITY_EVEN; break;
        case Parity::ODD:  parity = UART_PARITY_ODD;  break;
        case Parity::NO:   parity = UART_PARITY_NONE; break;
        default: ASSERT(false); break;
    }

    handle->Init.BaudRate     = static_cast<uint32_t>(config.mBaudrate);
    handle->Init.WordLength   = (config.mWordLength == WordLength::_8_BIT) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    handle->Init.Parity       = parity;
    handle->Init.StopBits     = (config.mStopBits == StopBits::_1_BIT) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    handle->Init.Mode         = UART_MODE_TX_RX;
    handle->Init.OverSampling = (config.mOverSampling == OverSampling::_8_TIMES) ? UART_OVERSAMPLING_8 : UART_OVERSAMPLING_16;
    handle->Init.HwFlowCtl    = (config.mUseHardwareFlowControl) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;

    if (HAL_UART_Init(handle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt
        const IRQn_Type irq = GetIRQn(mInstance);
        HAL_NVIC_DisableIRQ(irq);
        HAL_NVIC_ClearPendingIRQ(irq);
        HAL_NVIC_SetPriority(irq, config.mInterruptPriority, 0);
        HAL_NVIC_EnableIRQ(irq);

        return true;
    }
    return false;
}

// ToDo: comments
void Usart::Sleep() const
{
    // Disable interrupts
    const IRQn_Type irq = GetIRQn(mInstance);
    HAL_NVIC_DisableIRQ(irq);

    // ToDo: low power state, check recovery after sleep
}

//bool WriteDma(const uint8_t* src, size_t length, const std::function<void()>& refHandler);
//bool ReadDma(uint8_t* dest, size_t length, const std::function<void()>& refHandler);

/**
 * \brief   Write data using interrupts.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   refHander   Callback to call when write completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool Usart::WriteInterrupt(const uint8_t* src, size_t length, const std::function<void()>& refHandler)
{
    ASSERT(src);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    UART_HandleTypeDef* usartHandle = GetUsartHandle(mInstance);
    const auto index = GetIndex(usartHandle, UsartTransmissionType::Tx);
    ASSERT(index >= 0);
    callbackList[index] = refHandler;

    // Note: HAL_UART_Transmit_IT will check for src == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Transmit_IT(usartHandle, const_cast<uint8_t*>(src), length))
    {
        return true;
    }
    return false;
}

/**
 * \brief   Read data using interrupts.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   refHander   Callback to call when read completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool Usart::ReadInterrupt(uint8_t* dest, size_t length, const std::function<void()>& refHandler)
{
    ASSERT(dest);
    ASSERT(length > 0 && length <= MAX_TRANSMISSION_LENGTH);

    UART_HandleTypeDef* usartHandle = GetUsartHandle(mInstance);
    const auto index = GetIndex(usartHandle, UsartTransmissionType::Rx);
    ASSERT(index >= 0);
    callbackList[index] = refHandler;

    // Note: HAL_UART_Receive will check for dest == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Receive_IT(usartHandle, dest, length))
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

    UART_HandleTypeDef* handle = GetUsartHandle(mInstance);

    // Note: HAL_UART_Transmit will check for src == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Transmit(handle, const_cast<uint8_t*>(src), length, HAL_MAX_DELAY))
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

    UART_HandleTypeDef* handle = GetUsartHandle(mInstance);

    // Note: HAL_UART_Receive will check for dest == nullptr and size == 0 --> returns HAL_ERROR.

    if (HAL_OK == HAL_UART_Receive(handle, dest, length, HAL_MAX_DELAY))
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
 * \details This will create the relevant administration on the heap if
 *          required.
 * \param   instance    The USART instance to use.
 * \returns True if the administration could be created, else false.
 * \note    Asserts if the USART instance is invalid.
 */
bool Usart::SetInstance(const UsartInstance& instance)
{
    bool result = false;

    switch (instance)
    {
        case UsartInstance::USART_1:
            if (handleList[0] == nullptr) { handleList[0] = new(std::nothrow) UART_HandleTypeDef; }
            if (handleList[0] != nullptr)
            {
                handleList[0]->Instance = USART1;
                mInstance = USART1;
                result = true;
            }
            break;
        case UsartInstance::USART_2:
            if (handleList[1] == nullptr) { handleList[1] = new(std::nothrow) UART_HandleTypeDef; }
            if (handleList[1] != nullptr)
            {
                handleList[1]->Instance = USART2;
                mInstance = USART2;
                result = true;
            }
            break;
        case UsartInstance::USART_3:
            if (handleList[2] == nullptr) { handleList[2] = new(std::nothrow) UART_HandleTypeDef; }
            if (handleList[2] != nullptr)
            {
                handleList[2]->Instance = USART3;
                mInstance = USART3;
                result = true;
            }
            break;
        case UsartInstance::USART_6:
            if (handleList[3] == nullptr) { handleList[3] = new(std::nothrow) UART_HandleTypeDef; }
            if (handleList[3] != nullptr)
            {
                handleList[3]->Instance = USART6;
                mInstance = USART6;
                result = true;
            }
            break;

        default: ASSERT(false); break;      // Impossible selection
    }

    return result;
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
    // ToDo: check for error

    const auto index = GetIndex(handle, UsartTransmissionType::Tx);
    if (callbackList[index])
    {
        callbackList[index]();
    }
}

/**
 * \brief   ISR: handler to dispatch the USART RX completed interrupt into a RX
 *          callback.
 * \param   handle  The USART handle from which the RX ISR came.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* handle)
{
    // ToDo: check for error

    const auto index = GetIndex(handle, UsartTransmissionType::Rx);
    if (callbackList[index])
    {
        callbackList[index]();
    }
}

/**
 * \brief   ISR: route USART1 interrupts to either 'HAL_UART_TxCpltCallback' or
 *          'HAL_UART_RxCpltCallback'.
 */
extern "C" void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler( GetUsartHandle(USART1) );
}

/**
 * \brief   ISR: route USART2 interrupts to either 'HAL_UART_TxCpltCallback' or
 *          'HAL_UART_RxCpltCallback'.
 */
extern "C" void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler( GetUsartHandle(USART2) );
}

/**
 * \brief   ISR: route USART3 interrupts to either 'HAL_UART_TxCpltCallback' or
 *          'HAL_UART_RxCpltCallback'.
 */
extern "C" void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler( GetUsartHandle(USART3) );
}

/**
 * \brief   ISR: route USART6 interrupts to either 'HAL_UART_TxCpltCallback' or
 *          'HAL_UART_RxCpltCallback'.
 */
extern "C" void USART6_IRQHandler(void)
{
    HAL_UART_IRQHandler( GetUsartHandle(USART6) );
}
