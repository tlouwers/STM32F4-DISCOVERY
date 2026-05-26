/**
 * \file I2C.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   I2C
 *
 * \brief   I2C master peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/I2C
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/I2C/I2C.hpp"
#include "utility/Assert/Assert.h"
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
 * \brief   Fire and clear the callbackTx slot.
 * \param   i2c_callbacks   Structure containing the callbackTx to call.
 * \param   success         True if the transfer completed successfully,
 *                          false on bus error or aborted transfer.
 * \details The slot is cleared before invocation so the user handler is
 *          guaranteed to fire at most once per transfer, even if multiple
 *          HAL paths converge (e.g. ErrorCallback after a partial DMA).
 */
static void CallbackTxDone(I2CCallbacks& i2c_callbacks, bool success)
{
    if (i2c_callbacks.callbackTx)
    {
        std::function<void(bool)> handler;
        handler.swap(i2c_callbacks.callbackTx);
        handler(success);
    }
}

/**
 * \brief   Fire and clear the callbackRx slot.
 * \param   i2c_callbacks   Structure containing the callbackRx to call.
 * \param   success         True if the transfer completed successfully,
 *                          false on bus error or aborted transfer.
 * \details See CallbackTxDone for the once-per-transfer guarantee.
 */
static void CallbackRxDone(I2CCallbacks& i2c_callbacks, bool success)
{
    if (i2c_callbacks.callbackRx)
    {
        std::function<void(bool)> handler;
        handler.swap(i2c_callbacks.callbackRx);
        handler(success);
    }
}

/**
 * \brief   Blocking busy-wait of at least the requested microseconds.
 * \param   us  Number of microseconds to wait.
 * \details Uses the Cortex-M4 DWT cycle counter so the delay tracks the
 *          live core clock (SystemCoreClock) regardless of the board's
 *          PLL profile. Only the bit-banged bus recovery uses this, where
 *          ~100 kHz timing is non-critical (a slower clock is harmless).
 *          DWT->CYCCNT is never reset, so a concurrent DWT consumer (e.g.
 *          CpuWakeCounter) is left undisturbed; the wait is wrap-safe via
 *          unsigned subtraction.
 */
static void DelayMicroseconds(uint32_t us)
{
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0) { CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; }
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)            { DWT->CTRL       |= DWT_CTRL_CYCCNTENA_Msk;      }

    const uint32_t start  = DWT->CYCCNT;
    const uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while ((DWT->CYCCNT - start) < cycles) { }
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
}

/**
 * \brief   Destructor, disables interrupts.
 * \note    DisconnectCallbacks is also invoked unconditionally here as a
 *          safety net so a stale lambda capturing this object's `this`
 *          cannot be dispatched after destruction.
 */
I2C::~I2C()
{
    Sleep();
    DisconnectCallbacks();
}

/**
 * \brief   Initializes the I2C instance with the given configuration.
 * \param   config  The configuration for the I2C instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool I2C::Init(const IConfig& config)
{
    CheckAndEnablePeripheralClock(mInstance);

    EXPECT(config.ConfigId() == Config::Id());
    if (config.ConfigId() != Config::Id()) { return false; }

    const Config& cfg = static_cast<const Config&>(config);

    mHandle.Init.ClockSpeed      = (cfg.mBusSpeed == BusSpeed::NORMAL) ? 100000 : 400000;
    mHandle.Init.DutyCycle       = (cfg.mBusSpeed == BusSpeed::NORMAL) ? I2C_DUTYCYCLE_2 : I2C_DUTYCYCLE_16_9;
    mHandle.Init.OwnAddress1     = 0;
    mHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    mHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    mHandle.Init.OwnAddress2     = 0;
    mHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    mHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&mHandle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt on Event
        SetIRQn(GetIRQn(mInstance, IRQType::Event), cfg.mInterruptPriority, 0);

        // Configure NVIC to generate interrupt on Error
        SetIRQn(GetIRQn(mInstance, IRQType::Error), cfg.mInterruptPriority, 0);

        mI2CCallbacks.callbackEvent = [this]() { this->CallbackEvent(); };
        mI2CCallbacks.callbackError = [this]() { this->CallbackError(); };

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
 * \details Aborts ongoing transfers.
 * \returns True if I2C module could be put in sleep mode, else false.
 */
bool I2C::Sleep()
{
    // Abort only an in-flight master transfer; in any other state
    // HAL_I2C_Master_Abort_IT is a no-op. Result ignored so DeInit() runs.
    const HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&mHandle);
    if ((state == HAL_I2C_STATE_BUSY_TX) || (state == HAL_I2C_STATE_BUSY_RX))
    {
        HAL_I2C_Master_Abort_IT(&mHandle, static_cast<uint16_t>(mHandle.Devaddress));
    }

    if (HAL_I2C_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Event) );
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance, IRQType::Error) );

    DisconnectCallbacks();

    CheckAndDisablePeripheralClock(mInstance);
    return true;
}

/**
 * \brief   Recover a wedged I2C bus and restore the peripheral to its
 *          post-Init() state.
 * \param   scl     Pin id/port wired to this I2C's SCL line (the same
 *                  BoardConfig symbol used to set the bus up, e.g.
 *                  PIN_I2C1_SCL).
 * \param   sda     Pin id/port wired to this I2C's SDA line.
 * \returns True if the bus was recovered and the peripheral re-initialised;
 *          false if SDA stayed low after 9 clocks or re-init failed.
 * \details Combines two independent recoveries:
 *           1. Classic bus recovery -- with SCL/SDA driven as GPIO
 *              open-drain, up to 9 SCL pulses are issued while a slave
 *              clamps SDA low mid-byte, then a STOP is generated; this
 *              frees a stuck slave.
 *           2. STM32F4 ES0182 2.4.7 -- the analog filter can leave the
 *              BUSY flag stuck, blocking master-mode entry; the documented
 *              workaround disables PE, releases the lines via GPIO, then
 *              SWRSTs the peripheral.
 *          The pins are returned to AF4 open-drain (the I2C alternate
 *          function on every F407 I2C instance) and HAL_I2C_Init re-applies
 *          the original timing and re-enables the peripheral, so on success
 *          the bus is left exactly as after Init().
 * \note    Blocking; bit-bangs at roughly 100 kHz. Requires the peripheral
 *          to have been Init()'d -- the clock and handle must be live for
 *          the SWRST and re-init.
 */
bool I2C::RecoverBus(PinIdPort scl, PinIdPort sda)
{
    if (!mInitialized) { return false; }

    // ES0182 2.4.7 step 1: disable PE so the peripheral releases SCL/SDA.
    __HAL_I2C_DISABLE(&mHandle);

    // Drive both lines as GPIO open-drain, released high (the board's
    // external pull-ups float them high when not actively driven low).
    Pin sclPin(scl);
    Pin sdaPin(sda);
    sclPin.Configure(Level::HIGH, Drive::OPEN_DRAIN);
    sdaPin.Configure(Level::HIGH, Drive::OPEN_DRAIN);
    DelayMicroseconds(5);

    // Classic recovery: clock SCL until a slave clamping SDA lets go,
    // up to 9 pulses (one byte + ACK).
    for (uint8_t i = 0; (i < 9) && (sdaPin.Get() == Level::LOW); ++i)
    {
        sclPin.Set(Level::LOW);
        DelayMicroseconds(5);
        sclPin.Set(Level::HIGH);
        DelayMicroseconds(5);
    }

    const bool sdaReleased = (sdaPin.Get() == Level::HIGH);

    // Generate a STOP (SDA low->high while SCL high) so a slave that was
    // mid-transfer resynchronises to an idle bus.
    sdaPin.Set(Level::LOW);
    DelayMicroseconds(5);
    sclPin.Set(Level::HIGH);
    DelayMicroseconds(5);
    sdaPin.Set(Level::HIGH);
    DelayMicroseconds(5);

    // Hand the lines back to the I2C peripheral. I2C1/2/3 are all AF4 on
    // the STM32F407; HIGHZ/OPEN_DRAIN matches the Board pin setup.
    sclPin.Configure(Alternate::AF4, PullUpDown::HIGHZ, Mode::OPEN_DRAIN);
    sdaPin.Configure(Alternate::AF4, PullUpDown::HIGHZ, Mode::OPEN_DRAIN);

    // ES0182 2.4.7: software reset clears the stuck BUSY flag.
    SET_BIT(mHandle.Instance->CR1, I2C_CR1_SWRST);
    CLEAR_BIT(mHandle.Instance->CR1, I2C_CR1_SWRST);

    // Re-apply the original configuration; HAL_I2C_Init ends by enabling
    // the peripheral, leaving the bus exactly as after Init().
    if (HAL_I2C_Init(&mHandle) != HAL_OK) { return false; }

    return sdaReleased;
}

/**
 * \brief   Link a configured DMA stream into the I2C's Tx or Rx slot.
 * \param   dma     A DMA object that has been Configure()'d. The peripheral
 *                  slot to wire (hdmatx / hdmarx) is picked from the DMA's
 *                  Direction: MemoryToPeripheral wires Tx, PeripheralToMemory
 *                  wires Rx.
 * \returns True if the DMA was linked, false if dma was not configured or
 *          its Direction is not Tx/Rx (e.g. MemoryToMemory).
 */
bool I2C::LinkDma(DMA& dma)
{
    if (!dma.IsConfigured()) { return false; }

    switch (dma.GetDirection())
    {
        case DMA::Direction::MemoryToPeripheral:
            __HAL_LINKDMA(&mHandle, hdmatx, *dma.Handle());
            return true;
        case DMA::Direction::PeripheralToMemory:
            __HAL_LINKDMA(&mHandle, hdmarx, *dma.Handle());
            mDmaRx = &dma;
            return true;
        default:
            return false;
    }
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
bool I2C::WriteDMA(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void(bool)>& handler)
{
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == src) { return false; }
    if (0 == length)    { return false; }
    if (!mInitialized)  { return false; }
    if (nullptr == mHandle.hdmatx) { return false; }

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
bool I2C::ReadDMA(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void(bool)>& handler)
{
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == dest) { return false; }
    if (0 == length)     { return false; }
    if (!mInitialized)   { return false; }
    if (nullptr == mHandle.hdmarx) { return false; }

    mI2CCallbacks.callbackRx = handler;

    if (HAL_I2C_Master_Receive_DMA(&mHandle, slave, dest, length) != HAL_OK) { return false; }

    // HAL re-enables DMA_IT_HT regardless of the user's HalfBufferInterrupt
    // selection; reassert it.
    mDmaRx->EnforceHalfBufferInterruptSetting();
    return true;
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
bool I2C::WriteInterrupt(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void(bool)>& handler)
{
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == src) { return false; }
    if (0 == length)    { return false; }
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
bool I2C::ReadInterrupt(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void(bool)>& handler)
{
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == dest) { return false; }
    if (0 == length)     { return false; }
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
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == src) { return false; }
    if (0 == length)    { return false; }
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
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (nullptr == dest) { return false; }
    if (0 == length)     { return false; }
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
 * \brief   Enable the peripheral clock for the given I2C instance.
 * \param   instance    The I2C instance to enable the clock for.
 * \note    I2C1/2/3 all sit on APB1; the underlying __HAL_RCC_I2Cx_CLK_ENABLE
 *          macros target the correct bus.
 * \note    Asserts if not a valid I2C instance provided.
 */
void I2C::CheckAndEnablePeripheralClock(const I2CInstance& instance)
{
    switch (instance)
    {
        case I2CInstance::I2C_1: __HAL_RCC_I2C1_CLK_ENABLE(); break;
        case I2CInstance::I2C_2: __HAL_RCC_I2C2_CLK_ENABLE(); break;
        case I2CInstance::I2C_3: __HAL_RCC_I2C3_CLK_ENABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Disable the peripheral clock for the given I2C instance.
 * \param   instance    The I2C instance to disable the clock for.
 * \note    Asserts if not a valid I2C instance provided.
 */
void I2C::CheckAndDisablePeripheralClock(const I2CInstance& instance)
{
    switch (instance)
    {
        case I2CInstance::I2C_1: __HAL_RCC_I2C1_CLK_DISABLE(); break;
        case I2CInstance::I2C_2: __HAL_RCC_I2C2_CLK_DISABLE(); break;
        case I2CInstance::I2C_3: __HAL_RCC_I2C3_CLK_DISABLE(); break;
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
 * \brief   Lower level configuration for the I2C interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void I2C::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
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

/**
 * \brief   Clear every callback slot for this instance.
 * \details Called from Sleep() and from ~I2C() so a stale lambda capturing
 *          this object's `this` cannot be dispatched after the object goes
 *          away. Also clears any in-flight Tx/Rx user handler.
 */
void I2C::DisconnectCallbacks()
{
    mI2CCallbacks.callbackEvent = nullptr;
    mI2CCallbacks.callbackError = nullptr;
    mI2CCallbacks.callbackTx    = nullptr;
    mI2CCallbacks.callbackRx    = nullptr;
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: dispatch the I2C TX completed interrupt to the user handler.
 * \param   handle  The I2C handle from which the TX ISR came.
 * \details Checks ErrorCode defensively: HAL normally fires this callback
 *          only on success and routes failures to HAL_I2C_ErrorCallback,
 *          but we forward the actual outcome regardless so the contract
 *          documented on II2C holds even if HAL behaviour drifts.
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    const bool success = (handle->ErrorCode == HAL_I2C_ERROR_NONE);

    if      (handle->Instance == I2C1) { CallbackTxDone(i2c1_callbacks, success); }
    else if (handle->Instance == I2C2) { CallbackTxDone(i2c2_callbacks, success); }
    else if (handle->Instance == I2C3) { CallbackTxDone(i2c3_callbacks, success); }
}

/**
 * \brief   ISR: dispatch the I2C RX completed interrupt to the user handler.
 * \param   handle  The I2C handle from which the RX ISR came.
 * \details See HAL_I2C_MasterTxCpltCallback for the ErrorCode rationale.
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    const bool success = (handle->ErrorCode == HAL_I2C_ERROR_NONE);

    if      (handle->Instance == I2C1) { CallbackRxDone(i2c1_callbacks, success); }
    else if (handle->Instance == I2C2) { CallbackRxDone(i2c2_callbacks, success); }
    else if (handle->Instance == I2C3) { CallbackRxDone(i2c3_callbacks, success); }
}

/**
 * \brief   ISR: dispatch a bus-error completion to whichever async handler
 *          is in flight, with success=false.
 * \param   handle  The I2C handle from which the error ISR came.
 * \details HAL routes failed transfers here instead of the success
 *          callbacks, so without this override the user's handler would
 *          never fire after a NACK/AF/BERR and the caller would deadlock
 *          waiting on it. In normal sequential flow only one of the Tx/Rx
 *          slots is non-null at a time (each is cleared after firing), so
 *          we dispatch both unconditionally.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    if      (handle->Instance == I2C1) { CallbackTxDone(i2c1_callbacks, false); CallbackRxDone(i2c1_callbacks, false); }
    else if (handle->Instance == I2C2) { CallbackTxDone(i2c2_callbacks, false); CallbackRxDone(i2c2_callbacks, false); }
    else if (handle->Instance == I2C3) { CallbackTxDone(i2c3_callbacks, false); CallbackRxDone(i2c3_callbacks, false); }
}

/**
 * \brief   ISR: dispatch an aborted transfer to whichever async handler
 *          is in flight, with success=false.
 * \param   handle  The I2C handle from which the abort completion came.
 * \details Sleep() calls HAL_I2C_Master_Abort_IT, which fires this
 *          callback rather than the Cplt or Error paths. The handler
 *          fires with success=false so the caller is not stranded; if
 *          DisconnectCallbacks has already cleared the slots (the abort
 *          completes after Sleep tears down) the dispatch is a no-op.
 */
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef* handle)
{
    ASSERT(handle);

    if      (handle->Instance == I2C1) { CallbackTxDone(i2c1_callbacks, false); CallbackRxDone(i2c1_callbacks, false); }
    else if (handle->Instance == I2C2) { CallbackTxDone(i2c2_callbacks, false); CallbackRxDone(i2c2_callbacks, false); }
    else if (handle->Instance == I2C3) { CallbackTxDone(i2c3_callbacks, false); CallbackRxDone(i2c3_callbacks, false); }
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
 * \brief   ISR: route I2C2 Error interrupts to 'CallbackError'.
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
