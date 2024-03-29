/**
 * \file    SPI.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SPI
 *
 * \brief   SPI peripheral driver class - Master only.
 *
 * \note    The ChipSelect must be toggled outside this driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/SPI
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/SPI/SPI.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_spi.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static SPICallbacks spi1_callbacks {};
static SPICallbacks spi2_callbacks {};
static SPICallbacks spi3_callbacks {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   spi_callbacks   Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const SPICallbacks& spi_callbacks)
{
    if (spi_callbacks.callbackIRQ)
    {
        spi_callbacks.callbackIRQ();
    }
}

/**
 * \brief   Call the callbackTxRx, if configured.
 * \param   spi_callbacks   Structure containing the callbackTxRx to call.
 */
static void CallbackTxRxDone(const SPICallbacks& spi_callbacks)
{
    if (spi_callbacks.callbackTxRx)
    {
        spi_callbacks.callbackTxRx();
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal SPI instance administration.
 * \param   instance    The SPI instance to use.
 */
SPI::SPI(const SPIInstance& instance) :
    mInstance(instance),
    mSPICallbacks( (instance == SPIInstance::SPI_1) ? (spi1_callbacks) : ( (instance == SPIInstance::SPI_2) ? (spi2_callbacks) : (spi3_callbacks) ) ),
    mInitialized(false)
{
    SetInstance(instance);

    mSPICallbacks.callbackIRQ = [this]() { this->CallbackIRQ(); };
}

/**
 * \brief   Destructor, disabled interrupts.
 */
SPI::~SPI()
{
    Sleep();
}

/**
 * \brief   Initializes the SPI instance with the given configuration.
 * \param   config  The configuration for the SPI instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool SPI::Init(const IConfig& config)
{
    CheckAndEnableAHBPeripheralClock(mInstance);

    const Config& cfg = reinterpret_cast<const Config&>(config);

    if (cfg.mBusSpeed < 1) { return false; }                         // If BusSpeed too low then return.
    if (cfg.mBusSpeed > HAL_RCC_GetPCLK1Freq()) { return false; }    // If BusSpeed higher than peripheral clock then return.

    mHandle.Init.Mode              = SPI_MODE_MASTER;
    mHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    mHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
    mHandle.Init.CLKPolarity       = GetPolarity(cfg.mMode);
    mHandle.Init.CLKPhase          = GetPhase(cfg.mMode);
    mHandle.Init.NSS               = SPI_NSS_SOFT;
    mHandle.Init.BaudRatePrescaler = CalculatePrescaler(cfg.mBusSpeed);
    mHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    mHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    mHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    mHandle.Init.CRCPolynomial     = 0;

    if (HAL_SPI_Init(&mHandle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt
        SetIRQn(GetIRQn(mInstance), cfg.mInterruptPriority, 0);

        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if SPI is initialized.
 * \returns True if SPI is initialized, else false.
 */
bool SPI::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the SPI module in sleep mode.
 * \details Aborts ongoing transfers.
 * \returns True if SPI module could be put in sleep mode, else false.
 */
bool SPI::Sleep()
{
    // For Int. and DMA started transfers. Not handling result as to reach DeInit().
    HAL_SPI_Abort(&mHandle);

    mInitialized = false;

    if (HAL_SPI_DeInit(&mHandle) == HAL_OK)
    {
        CheckAndDisableAHBPeripheralClock(mInstance);
        return true;
    }
    return false;
}

/**
 * \brief   Get the handle to the peripheral.
 * \returns The handle to the peripheral.
 */
const SPI_HandleTypeDef* SPI::GetPeripheralHandle() const
{
    return &mHandle;
}

/**
 * \brief   Get the pointer to the Dma Tx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Tx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& SPI::GetDmaTxHandle()
{
    return mHandle.hdmatx;
}

/**
 * \brief   Get the pointer to the Dma Rx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Rx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& SPI::GetDmaRxHandle()
{
    return mHandle.hdmarx;
}

/**
 * \brief   Write data using DMA.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   handler     Callback to call when write completed.
 * \returns True if the transaction could be started, else false. Returns false if no DMA is setup for Tx.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool SPI::WriteDMA(const uint8_t* src, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }
    if (mHandle.hdmatx == nullptr) { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_Transmit_DMA(&mHandle, const_cast<uint8_t*>(src), length) == HAL_OK);
}

/**
 * \brief   Write and Read data using DMA.
 * \param   src         Pointer to buffer with data to write.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false. Returns false
 *          if no DMA is setup for Rx.
 * \note    Asserts if src or dest is nullptr or length invalid.
 * \note    Write and Read happen at the same time, hence both buffers are the
 *          same sime.
 */
bool SPI::WriteReadDMA(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(src);
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr)  { return false; }
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }
    if (mHandle.hdmatx == nullptr) { return false; }
    if (mHandle.hdmarx == nullptr) { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_TransmitReceive_DMA(&mHandle, const_cast<uint8_t*>(src), dest, length) == HAL_OK);
}

/**
 * \brief   Read data using DMA.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false. Returns false
 *          if no DMA is setup for Rx.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool SPI::ReadDMA(uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }
    if (mHandle.hdmarx == nullptr) { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_Receive_DMA(&mHandle, dest, length) == HAL_OK);
}

/**
 * \brief   Write data using interrupts.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \param   handler     Callback to call when write completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool SPI::WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_Transmit_IT(&mHandle, const_cast<uint8_t*>(src), length) == HAL_OK);
}

/**
 * \brief   Write and Read data using interrupts.
 * \param   src         Pointer to buffer with data to write.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if src or dest is nullptr or length invalid.
 * \note    Write and Read happen at the same time, hence both buffers are the
 *          same sime.
 */
bool SPI::WriteReadInterrupt(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(src);
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr)  { return false; }
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_TransmitReceive_IT(&mHandle, const_cast<uint8_t*>(src), dest, length) == HAL_OK);
}

/**
 * \brief   Read data using interrupts.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \param   handler     Callback to call when read completed.
 * \returns True if the transaction could be started, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool SPI::ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    mSPICallbacks.callbackTxRx = handler;

    return (HAL_SPI_Receive_IT(&mHandle, dest, length) == HAL_OK);
}

/**
 * \brief   Write data using blocking call.
 * \param   src         Pointer to buffer with data to write.
 * \param   length      Length of the data to write in bytes.
 * \returns True if the write was successful, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool SPI::WriteBlocking(const uint8_t* src, uint16_t length)
{
    EXPECT(src);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr) { return false; }
    if (length == 0)    { return false; }
    if (!mInitialized)  { return false; }

    return (HAL_SPI_Transmit(&mHandle, const_cast<uint8_t*>(src), length, HAL_MAX_DELAY) == HAL_OK);
}

/**
 * \brief   Write and Read data using blocking call.
 * \param   src         Pointer to buffer with data to write.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \returns True if the read was successful, else false.
 * \note    Asserts if src or dest is nullptr or length invalid.
 * \note    Write and Read happen at the same time, hence both buffers are the
 *          same sime.
 */
bool SPI::WriteReadBlocking(const uint8_t* src, uint8_t* dest, uint16_t length)
{
    EXPECT(src);
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (src == nullptr)  { return false; }
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    return (HAL_SPI_TransmitReceive(&mHandle, const_cast<uint8_t*>(src), dest, length, HAL_MAX_DELAY) == HAL_OK);
}

/**
 * \brief   Read data using blocking call.
 * \param   dest        Pointer to buffer where to store the read data.
 * \param   length      Length of the data to read in bytes.
 * \returns True if the read was successful, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool SPI::ReadBlocking(uint8_t* dest, uint16_t length)
{
    EXPECT(dest);
    EXPECT(length > 0);

    // Note: HAL will NOT check on parameters
    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (!mInitialized)   { return false; }

    return (HAL_SPI_Receive(&mHandle, dest, length, HAL_MAX_DELAY) == HAL_OK);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the SPI instance into internal administration.
 * \param   instance    The SPI instance to use.
 * \note    Asserts if the SPI instance is invalid.
 */
void SPI::SetInstance(const SPIInstance& instance)
{
    switch (instance)
    {
        case SPIInstance::SPI_1: mHandle.Instance = SPI1; break;
        case SPIInstance::SPI_2: mHandle.Instance = SPI2; break;
        case SPIInstance::SPI_3: mHandle.Instance = SPI3; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB peripheral clock for the SPI
 *          instance is enabled, if not enable it.
 * \param   instance    The SPI instance to enable the clock for.
 * \note    Asserts if not a valid SPI instance provided.
 */
void SPI::CheckAndEnableAHBPeripheralClock(const SPIInstance& instance)
{
    switch (instance)
    {
        case SPIInstance::SPI_1: if (__HAL_RCC_SPI1_IS_CLK_DISABLED()) { __HAL_RCC_SPI1_CLK_ENABLE(); } break;
        case SPIInstance::SPI_2: if (__HAL_RCC_SPI2_IS_CLK_DISABLED()) { __HAL_RCC_SPI2_CLK_ENABLE(); } break;
        case SPIInstance::SPI_3: if (__HAL_RCC_SPI3_IS_CLK_DISABLED()) { __HAL_RCC_SPI3_CLK_ENABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB peripheral clock for the SPI
 *          instance is enabled, if so disable it.
 * \param   instance    The SPI instance to disable the clock for.
 * \note    Asserts if not a valid SPI instance provided.
 */
void SPI::CheckAndDisableAHBPeripheralClock(const SPIInstance& instance)
{
    switch (instance)
    {
        case SPIInstance::SPI_1: if (__HAL_RCC_SPI1_IS_CLK_ENABLED()) { __HAL_RCC_SPI1_CLK_DISABLE(); } break;
        case SPIInstance::SPI_2: if (__HAL_RCC_SPI2_IS_CLK_ENABLED()) { __HAL_RCC_SPI2_CLK_DISABLE(); } break;
        case SPIInstance::SPI_3: if (__HAL_RCC_SPI3_IS_CLK_ENABLED()) { __HAL_RCC_SPI3_CLK_DISABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Get the polarity for the given mode.
 * \param   mode    The mode to get the polarity for.
 * \returns The polarity if successful, else 0.
 */
uint32_t SPI::GetPolarity(const Mode& mode)
{
    uint32_t polarity = SPI_POLARITY_LOW;

    switch (mode)
    {
        case Mode::_0: polarity = SPI_POLARITY_LOW;  break;
        case Mode::_1: polarity = SPI_POLARITY_LOW;  break;
        case Mode::_2: polarity = SPI_POLARITY_HIGH; break;
        case Mode::_3: polarity = SPI_POLARITY_HIGH; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return polarity;
}

/**
 * \brief   Get the phase for the given mode.
 * \param   mode    The mode to get the phase for.
 * \returns The phase if successful, else 0.
 */
uint32_t SPI::GetPhase(const Mode& mode)
{
    uint32_t phase = SPI_PHASE_1EDGE;

    switch (mode)
    {
        case Mode::_0: phase = SPI_PHASE_1EDGE; break;
        case Mode::_1: phase = SPI_PHASE_2EDGE; break;
        case Mode::_2: phase = SPI_PHASE_1EDGE; break;
        case Mode::_3: phase = SPI_PHASE_2EDGE; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return phase;
}

/**
 * \brief   Calculate the SPI bus prescale value.
 * \param   busSpeed    The desired bus speed to use.
 * \returns Prescaler which takes the peripheral clock into account and tries
 *          to get the closest prescaler value to meet that speed.
 */
uint32_t SPI::CalculatePrescaler(uint32_t busSpeed)
{
    uint32_t prescaler = HAL_RCC_GetPCLK1Freq() / busSpeed;
         if (prescaler >= 256) { prescaler = SPI_BAUDRATEPRESCALER_256; }
    else if (prescaler >= 128) { prescaler = SPI_BAUDRATEPRESCALER_128; }
    else if (prescaler >=  64) { prescaler = SPI_BAUDRATEPRESCALER_64;  }
    else if (prescaler >=  32) { prescaler = SPI_BAUDRATEPRESCALER_32;  }
    else if (prescaler >=  16) { prescaler = SPI_BAUDRATEPRESCALER_16;  }
    else if (prescaler >=   8) { prescaler = SPI_BAUDRATEPRESCALER_8;   }
    else if (prescaler >=   4) { prescaler = SPI_BAUDRATEPRESCALER_4;   }
    else if (prescaler >=   2) { prescaler = SPI_BAUDRATEPRESCALER_2;   }
    else                       { prescaler = 1;                         }

    return prescaler;
}

/**
 * \brief   Get the IRQ belonging to the SPI.
 * \param   instance    The SPI instance to get the IRQ for.
 * \returns The interrupt line IRQ to which the SPI belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid SPI instance provided.
 */
IRQn_Type SPI::GetIRQn(const SPIInstance& instance)
{
    switch (instance)
    {
        case SPIInstance::SPI_1: return SPI1_IRQn; break;
        case SPIInstance::SPI_2: return SPI2_IRQn; break;
        case SPIInstance::SPI_3: return SPI3_IRQn; break;
        default: ASSERT(false); while(1) { __NOP(); } return SPI1_IRQn; break;      // Impossible selection
    }
}

/**
 * \brief   Lower level configuration for the SPI interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void SPI::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
}

/**
 * \brief   Generic SPI IRQ callback. Will propagate other interrupts.
 */
void SPI::CallbackIRQ()
{
    HAL_SPI_IRQHandler(&mHandle);
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the SPI TX completed interrupt into a TX
 *          callback.
 * \param   handle  The SPI handle from which the TX ISR came.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    if (handle->Instance == SPI1) { CallbackTxRxDone(spi1_callbacks); }
    if (handle->Instance == SPI2) { CallbackTxRxDone(spi2_callbacks); }
    if (handle->Instance == SPI3) { CallbackTxRxDone(spi3_callbacks); }
}

/**
 * \brief   ISR: handler to dispatch the SPI RX completed interrupt into a RX
 *          callback.
 * \param   handle  The SPI handle from which the RX ISR came.
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* handle)
{
    ASSERT(handle);

    // ToDo: check for error

    if (handle->Instance == SPI1) { CallbackTxRxDone(spi1_callbacks); }
    if (handle->Instance == SPI2) { CallbackTxRxDone(spi2_callbacks); }
    if (handle->Instance == SPI3) { CallbackTxRxDone(spi3_callbacks); }
}

/**
 * \brief   ISR: route SPI1 interrupts to 'CallbackIRQ'.
 */
extern "C" void SPI1_IRQHandler(void)
{
    CallbackIRQ(spi1_callbacks);
}

/**
 * \brief   ISR: route SPI2 interrupts to 'CallbackIRQ'.
 */
extern "C" void SPI2_IRQHandler(void)
{
    CallbackIRQ(spi2_callbacks);
}

/**
 * \brief   ISR: route SPI3 interrupts to 'CallbackIRQ'.
 */
extern "C" void SPI3_IRQHandler(void)
{
    CallbackIRQ(spi3_callbacks);
}
