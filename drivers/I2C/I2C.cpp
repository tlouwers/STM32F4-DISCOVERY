/**
 * \file I2C.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   I2C peripheral driver class.
 *
 * \details This implements 'master' only, for '7-bit' addressing.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/I2C
 *
 * \details Intended use is to provide an easier means to work with the I2C
 *          peripheral. This class assumes the pins to use for the I2C are
 *          already configured.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        10-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "I2C.hpp"
#include "utility/SlimAssert.h"
#include "stm32f4xx_hal_i2c.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static I2CCallbacks i2c1_callbacks {};
static I2CCallbacks i2c2_callbacks {};
static I2CCallbacks i2c3_callbacks {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackEvent, if configured.
 * \param   i2c_callbacks   Structure containing the callbackEvent to call.
 */
static void CallbackEvent(const I2CCallbacks& i2c_callbacks)
{
    if (i2c_callbacks.callbackEvent)
    {
        i2c_callbacks.callbackEvent();
    }
}

/**
 * \brief   Call the callbackError, if configured.
 * \param   i2c_callbacks   Structure containing the callbackError to call.
 */
static void CallbackError(const I2CCallbacks& i2c_callbacks)
{
    if (i2c_callbacks.callbackError)
    {
        i2c_callbacks.callbackError();
    }
}

/**
 * \brief   Call the callbackTx, if configured.
 * \param   i2c_callbacks   Structure containing the callbackTx to call.
 */
static void CallbackTxDone(const I2CCallbacks& i2c_callbacks)
{
    if (i2c_callbacks.callbackTx)
    {
        i2c_callbacks.callbackTx();
    }
}

/**
 * \brief   Call the callbackRx, if configured.
 * \param   i2c_callbacks   Structure containing the callbackRx to call.
 */
static void CallbackRxDone(const I2CCallbacks& i2c_callbacks)
{
    if (i2c_callbacks.callbackRx)
    {
        i2c_callbacks.callbackRx();
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal I2C instance administration.
 * \param   instance    The I2C instance to use.
 */
I2C::I2C(const I2CInstance& instance) :
    mInstance(instance),
    mI2CCallbacks( (instance == I2CInstance::I2C_1) ? (i2c1_callbacks) : ( (instance == I2CInstance::I2C_2) ? (i2c2_callbacks) : (i2c3_callbacks) ) ),
    mInitialized(false)
{
    SetInstance(instance);

    mI2CCallbacks.callbackEvent = [this]() { this->CallbackEvent(); };
    mI2CCallbacks.callbackError = [this]() { this->CallbackError(); };
}

/**
 * \brief   Destructor, disabled interrupts.
 */
I2C::~I2C()
{
    // Disable interrupts
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Event) );
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Error) );

    mInitialized = false;
}

/**
 * \brief   Initializes the I2C instance with the given configuration.
 * \param   config  The configuration for the I2C instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool I2C::Init(const Config& config)
{
    CheckAndEnableAHB1PeripheralClock(mInstance);

    mHandle.Init.ClockSpeed      = (config.mBusSpeed == BusSpeed::NORMAL) ? 100000 : 400000;
    mHandle.Init.DutyCycle       = (config.mBusSpeed == BusSpeed::NORMAL) ? I2C_DUTYCYCLE_2 : I2C_DUTYCYCLE_16_9;
    mHandle.Init.OwnAddress1     = 0;
    mHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    mHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    mHandle.Init.OwnAddress2     = 0;
    mHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    mHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&mHandle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt on Event
        const IRQn_Type irq_event = GetIRQn(mInstance, IRQType::Event);
        HAL_NVIC_DisableIRQ(irq_event);
        HAL_NVIC_ClearPendingIRQ(irq_event);
        HAL_NVIC_SetPriority(irq_event, config.mInterruptPriority, 0);
        HAL_NVIC_EnableIRQ(irq_event);

        // Configure NVIC to generate interrupt on Error
        const IRQn_Type irq_error = GetIRQn(mInstance, IRQType::Error);
        HAL_NVIC_DisableIRQ(irq_error);
        HAL_NVIC_ClearPendingIRQ(irq_error);
        HAL_NVIC_SetPriority(irq_error, config.mInterruptPriority, 0);
        HAL_NVIC_EnableIRQ(irq_error);

        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if I2C is initialized.
 * \returns True if I2C is initialized, else false.
 */
bool I2C::IsInit() const
{
    return mInitialized;
}

/**
 * \brief    Puts the I2C module in sleep mode.
 */
void I2C::Sleep()
{
    // Disable interrupts
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Event) );
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Error) );

    mInitialized = false;

    // ToDo: low power state, check recovery after sleep
}

/**
 * \brief   Get the handle to the peripheral.
 * \returns The handle to the peripheral.
 */
const I2C_HandleTypeDef* I2C::GetPeripheralHandle() const
{
    return &mHandle;
}

/**
 * \brief   Get the pointer to the Dma Tx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Tx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& I2C::GetDmaTxHandle()
{
    return mHandle.hdmatx;
}

/**
 * \brief   Get the pointer to the Dma Rx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Rx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& I2C::GetDmaRxHandle()
{
    return mHandle.hdmarx;
}

/**
 * \brief   Write data using DMA.
 * \param   slave       The slave address.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   handler     Callback to call when write completed.
 * \returns True if the transaction could be started, else false. Returns false if no DMA is setup for Tx.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool I2C::WriteDMA(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler)
{
    ASSERT(src);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }
    if (mHandle.hdmatx == nullptr) { return false; }

    mI2CCallbacks.callbackTx = handler;

    return (HAL_I2C_Master_Transmit_DMA(&mHandle, slave, const_cast<uint8_t*>(src), length) == HAL_OK);
}

/**
 * \brief   Read data using DMA.
 * \param   slave       The slave address.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false. Returns false
 *          if no DMA is setup for Rx.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool I2C::ReadDMA(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    ASSERT(dest);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }
    if (mHandle.hdmarx == nullptr) { return false; }

    mI2CCallbacks.callbackRx = handler;

    return (HAL_I2C_Master_Receive_DMA(&mHandle, slave, dest, length) == HAL_OK);
}

/**
 * \brief   Write data using interrupts.
 * \param   slave       The slave address.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   handler     Callback to call when write completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool I2C::WriteInterrupt(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler)
{
    ASSERT(src);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }

    mI2CCallbacks.callbackTx = handler;

    return (HAL_I2C_Master_Transmit_IT(&mHandle, slave, const_cast<uint8_t*>(src), length) == HAL_OK);
}

/**
 * \brief   Read data using interrupts.
 * \param   slave       The slave address.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool I2C::ReadInterrupt(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    ASSERT(dest);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    mI2CCallbacks.callbackRx = handler;

    return (HAL_I2C_Master_Receive_IT(&mHandle, slave, dest, length) == HAL_OK);
}

/**
 * \brief   Write data using blocking call.
 * \param   slave       The slave address.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \returns True if the write was successful, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool I2C::WriteBlocking(uint8_t slave, const uint8_t* src, uint16_t length)
{
    ASSERT(src);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }

    return (HAL_I2C_Master_Transmit(&mHandle, slave, const_cast<uint8_t*>(src), length, HAL_MAX_DELAY) == HAL_OK);
}

/**
 * \brief   Read data using blocking call.
 * \param   slave       The slave address.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \returns True if the read was successful, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool I2C::ReadBlocking(uint8_t slave, uint8_t* dest, uint16_t length)
{
    ASSERT(dest);
    ASSERT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    return (HAL_I2C_Master_Receive(&mHandle, slave, dest, length, HAL_MAX_DELAY) == HAL_OK);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the I2C instance into internal administration.
 * \param   instance    The I2C instance to use.
 * \note    Asserts if the I2C instance is invalid.
 */
void I2C::SetInstance(const I2CInstance& instance)
{
    switch (instance)
    {
        case I2CInstance::I2C_1: mHandle.Instance = I2C1; break;
        case I2CInstance::I2C_2: mHandle.Instance = I2C2; break;
        case I2CInstance::I2C_3: mHandle.Instance = I2C3; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the I2C
 *          instance is enabled, if not enable it.
 * \param   instance    The I2C instance to enable the clock for.
 * \note    Asserts if not a valid I2C instance provided.
 */
void I2C::CheckAndEnableAHB1PeripheralClock(const I2CInstance& instance)
{
    switch (instance)
    {
        case I2CInstance::I2C_1: if (__HAL_RCC_I2C1_IS_CLK_DISABLED()) { __HAL_RCC_I2C1_CLK_ENABLE(); } break;
        case I2CInstance::I2C_2: if (__HAL_RCC_I2C2_IS_CLK_DISABLED()) { __HAL_RCC_I2C2_CLK_ENABLE(); } break;
        case I2CInstance::I2C_3: if (__HAL_RCC_I2C3_IS_CLK_DISABLED()) { __HAL_RCC_I2C3_CLK_ENABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Get the IRQ belonging to the I2C.
 * \param   instance    The I2C instance to get the IRQ for.
 * \param   type        The IRQ type to get.
 * \returns The interrupt line IRQ to which the I2C belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid I2C instance provided.
 */
IRQn_Type I2C::GetIRQn(const I2CInstance& instance, IRQType type)
{
    if (type == IRQType::Event)
    {
        switch (instance)
        {
            case I2CInstance::I2C_1: return I2C1_EV_IRQn; break;
            case I2CInstance::I2C_2: return I2C2_EV_IRQn; break;
            case I2CInstance::I2C_3: return I2C3_EV_IRQn; break;
            default: ASSERT(false); while(1) { __NOP(); } return I2C1_EV_IRQn; break;      // Impossible selection
        }
    }
    else
    {
        switch (instance)
        {
            case I2CInstance::I2C_1: return I2C1_ER_IRQn; break;
            case I2CInstance::I2C_2: return I2C2_ER_IRQn; break;
            case I2CInstance::I2C_3: return I2C3_ER_IRQn; break;
            default: ASSERT(false); while(1) { __NOP(); } return I2C1_ER_IRQn; break;      // Impossible selection
        }
    }
}

/**
 * \brief   Generic I2C Event callback. Will propagate other interrupts.
 */
void I2C::CallbackEvent()
{
    HAL_I2C_EV_IRQHandler(&mHandle);
}

/**
 * \brief   Generic I2C Error callback. Will propagate other interrupts.
 */
void I2C::CallbackError()
{
    HAL_I2C_ER_IRQHandler(&mHandle);
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the I2C TX completed interrupt into a TX
 *          callback.
 * \param   handle  The I2C handle from which the TX ISR came.
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    if (handle->Instance == I2C1) { CallbackTxDone(i2c1_callbacks); }
    if (handle->Instance == I2C2) { CallbackTxDone(i2c2_callbacks); }
    if (handle->Instance == I2C3) { CallbackTxDone(i2c3_callbacks); }
}

/**
 * \brief   ISR: handler to dispatch the I2C RX completed interrupt into a RX
 *          callback.
 * \param   handle  The I2C handle from which the RX ISR came.
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    if (handle->Instance == I2C1) { CallbackRxDone(i2c1_callbacks); }
    if (handle->Instance == I2C2) { CallbackRxDone(i2c2_callbacks); }
    if (handle->Instance == I2C3) { CallbackRxDone(i2c3_callbacks); }
}

/**
 * \brief   ISR: route I2C1 Event interrupts to 'CallbackEvent'.
 */
extern "C" void I2C1_EV_IRQHandler(void)
{
    CallbackEvent(i2c1_callbacks);
}

/**
 * \brief   ISR: route I2C1 Error interrupts to 'CallbackError'.
 */
extern "C" void I2C1_ER_IRQHandler(void)
{
    CallbackError(i2c1_callbacks);
}

/**
 * \brief   ISR: route I2C2 Event interrupts to 'CallbackEvent'.
 */
extern "C" void I2C2_EV_IRQHandler(void)
{
    CallbackEvent(i2c2_callbacks);
}

/**
 * \brief   ISR: route I2C3 Error interrupts to 'CallbackError'.
 */
extern "C" void I2C2_ER_IRQHandler(void)
{
    CallbackError(i2c2_callbacks);
}

/**
 * \brief   ISR: route I2C3 Event interrupts to 'CallbackEvent'.
 */
extern "C" void I2C3_EV_IRQHandler(void)
{
    CallbackEvent(i2c3_callbacks);
}

/**
 * \brief   ISR: route I2C3 Error interrupts to 'CallbackError'.
 */
extern "C" void I2C3_ER_IRQHandler(void)
{
    CallbackError(i2c3_callbacks);
}
