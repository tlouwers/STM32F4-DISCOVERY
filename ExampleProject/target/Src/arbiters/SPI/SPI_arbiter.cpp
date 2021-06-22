/**
 * \file    SPI_arbiter.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SPI_arbiter
 *
 * \brief   Arbiter class for SPI (master) implementation.
 *
 * \note    The ChipSelect must be toggled outside this driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/arbiters/SPI
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "arbiters/SPI/SPI_arbiter.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, creates SPI (master) driver object and prepares
 *          it for use.
 * \param   instance    The SPI instance to use.
 */
SPI_arbiter::SPI_arbiter(const SPIInstance& instance) :
    mSpiMaster(instance),
    mBusy(false)
{
    mBuffer.clear();
}

/**
 * \brief   Destructor, clears internal map with callbacks.
 */
SPI_arbiter::~SPI_arbiter()
{
    mBusy = false;

    mLock.clear(std::memory_order_release);

    mBuffer.clear();
}

/**
 * \brief   Initializes the SPI (master) bus.
 * \param   config  Configuration of the SPI bus.
 * \returns True if init successful, else false.
 */
bool SPI_arbiter::Init(const IConfig& config)
{
    return (mSpiMaster.Init(config)) ? true : false;
}

/**
 * \brief   Check if SPI (master) is initialized or not.
 * \returns True if initialized, else false.
 */
bool SPI_arbiter::IsInit() const
{
    return mSpiMaster.IsInit();
}

/**
 * \brief   Put SPI (master) Arbiter module to sleep, first wait until all messages are sent,
 *          then clear the buffer and put SPI (master) bus to sleep.
 * \returns True if SPI module could be put in sleep mode, else false.
 * \remarks When timeout is reached the SPI (master) bus is forced to sleep regardless.
 */
bool SPI_arbiter::Sleep()
{
    while (mBusy) { __NOP(); }                                          // Blocking wait until we can use the bus, prevent loop from being optimized away.

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    mBuffer.clear();

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }                                      // Restore global interrupts

    return mSpiMaster.Sleep();
}

/**
 * \brief   Get the handle to the peripheral.
 * \returns The handle to the peripheral.
 */
const SPI_HandleTypeDef* SPI_arbiter::GetPeripheralHandle() const
{
    return mSpiMaster.GetPeripheralHandle();
}

/**
 * \brief   Get the pointer to the Dma Tx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Tx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& SPI_arbiter::GetDmaTxHandle()
{
    return const_cast<DMA_HandleTypeDef*&>(mSpiMaster.GetPeripheralHandle()->hdmatx);
}

/**
 * \brief   Get the pointer to the Dma Rx handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The Dma Rx handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& SPI_arbiter::GetDmaRxHandle()
{
    return const_cast<DMA_HandleTypeDef*&>(mSpiMaster.GetPeripheralHandle()->hdmarx);
}

/**
 * \brief   Pass thru method to SPI WriteDMA method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   src         The buffer with the data to write.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is sent.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::WriteDMA(const uint8_t* src, uint16_t length, const std::function<void()>& handler) 
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::WriteDMA;
        element.src          = src;
        element.dest         = nullptr;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }                                      // Restore global interrupts

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.WriteDMA(src, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI WriteReadDMA method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   src         The buffer with the data to write.
 * \param   dest        The buffer to store the read data.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is received.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::WriteReadDMA(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::WriteReadDMA;
        element.src          = src;
        element.dest         = dest;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }  

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.WriteReadDMA(src, dest, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI ReadDMA method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   dest        The buffer to store the read data.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is sent.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::ReadDMA(uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::ReadDMA;
        element.src          = nullptr;
        element.dest         = dest;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }  

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.ReadDMA(dest, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI WriteInterrupt method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   src         The buffer with the data to write.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is sent.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler) 
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::WriteInterrupt;
        element.src          = src;
        element.dest         = nullptr;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }                                      // Restore global interrupts

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.WriteInterrupt(src, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI WriteReadInterrupt method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   src         The buffer with the data to write.
 * \param   dest        The buffer to store the read data.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is received.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::WriteReadInterrupt(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::WriteReadInterrupt;
        element.src          = src;
        element.dest         = dest;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }  

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.WriteReadInterrupt(src, dest, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI ReadInterrupt method.
 * \details If the bus is busy the pointers to the data are queued and send when
 *          the bus becomes available.
 * \param   dest        The buffer to store the read data.
 * \param   length      The length of the message.
 * \param   handler     Callback to call when data is sent.
 * \note    Asserts when SPI (master) is not yet initialized.
 */
bool SPI_arbiter::ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void()>& handler)
{
    EXPECT(mSpiMaster.IsInit());

    ArbiterElementSpiMaster element;
        element.request_type = RequestType::ReadInterrupt;
        element.src          = nullptr;
        element.dest         = dest;
        element.length       = length;
        element.callbackDone = handler;

    // The lock is needed to make a multiple producer of the CircularBuffer
    //  (which is single producer thread safe only).
    // The DataRequestHandler is the single consumer, there no lock is
    //  needed (or allowed! as it is inside an ISR).

    const uint32_t prim = __get_PRIMASK();                              // Disable global interrupts to prevent race condition
    __disable_irq();
    while (mLock.test_and_set(std::memory_order_acquire)) { __NOP(); }  // Acquire lock - start of critical section, prevent loop from being optimized away.

    bool result = mBuffer.push(element);
    EXPECT(result);

    mLock.clear(std::memory_order_release);                             // Release lock - end of critical section
    if (!prim) { __enable_irq(); }  

    // Start the transmission, if not busy yet
    if (mSpiMaster.IsInit())
    {
        if (!mBusy)
        {
            mBusy = true;

            // Reroute the data received callback to the arbiter
            result = mSpiMaster.ReadInterrupt(dest, length, [this]() { this->DataRequestHandler(); });
            EXPECT(result);
        }
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI WriteBlocking method.
 * \details If the bus is busy a blocking wait is done until the bus becomes
 *          available, then the data is send. If the bus does not become
 *          available the sending of data is skipped.
 * \param   src     The buffer with the data to write.
 * \param   length  The length of the message.
 * \returns True if the message was sent, else false.
 */
bool SPI_arbiter::WriteBlocking(const uint8_t* src, uint16_t length)
{
    EXPECT(mSpiMaster.IsInit());

    bool result = false;

    if (mSpiMaster.IsInit())
    {
        while (mBusy) { __NOP(); }      // Blocking wait until we can use the bus, prevent loop from being optimized away.

        mBusy = true;
        result = mSpiMaster.WriteBlocking(src, length);
        EXPECT(result);
        mBusy = false;
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI WriteReadBlocking method.
 * \details If the bus is busy a blocking wait is done until the bus becomes
 *          available, then the data is send. If the bus does not become
 *          available the sending of data is skipped.
 * \param   src     The buffer with the data to write.
 * \param   dest    The buffer to store the read data.
 * \param   length  The length of the message.
 * \returns True if the message was sent, else false.
 */
bool SPI_arbiter::WriteReadBlocking(const uint8_t* src, uint8_t* dest, uint16_t length)
{
    EXPECT(mSpiMaster.IsInit());

    bool result = false;

    if (mSpiMaster.IsInit())
    {
        while (mBusy) { __NOP(); }      // Blocking wait until we can use the bus, prevent loop from being optimized away.

        mBusy = true;
        result = mSpiMaster.WriteReadBlocking(src, dest, length);
        EXPECT(result);
        mBusy = false;
    }

    return result;
}

/**
 * \brief   Pass thru method to SPI ReadBlocking method.
 * \details If the bus is busy a blocking wait is done until the bus becomes
 *          available, then the data is send. If the bus does not become
 *          available the sending of data is skipped.
 * \param   dest    The buffer to store the read data.
 * \param   length  The length of the message.
 * \returns True if the message was sent, else false.
 */
bool SPI_arbiter::ReadBlocking(uint8_t* dest, uint16_t length)
{
    EXPECT(mSpiMaster.IsInit());

    bool result = false;

    if (mSpiMaster.IsInit())
    {
        while (mBusy) { __NOP(); }      // Blocking wait until we can use the bus, prevent loop from being optimized away.

        mBusy = true;
        result = mSpiMaster.ReadBlocking(dest, length);
        EXPECT(result);
        mBusy = false;
    }

    return result;
}


/************************************************************************/
/* Private Members                                                      */
/************************************************************************/
/**
 * \brief   Handler which is called when either TX or RX is done
 *          for SPI, allowing arbitration on the bus.
 * \details Checks if there is queued data, if so send it, else
 *          release the bus.
 */
void SPI_arbiter::DataRequestHandler()
{
    ArbiterElementSpiMaster element;

    // Since we are the only consumer, using the CircularBuffer class
    // provides thread safety.

    // Remove the element from the queue, handled.
    mBuffer.pop(element);

    // Call the callback, if there was one set.
    if (element.callbackDone)
    {
        element.callbackDone();
    }

    bool result = false;

    // Check if we need to handle the next item.
    if (mBuffer.peek(element))
    {
        switch (element.request_type)
        {
            // Reroute the data to send callback to the arbiter
            case RequestType::WriteDMA:
                result = mSpiMaster.WriteDMA(element.src, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            case RequestType::WriteReadDMA:
                result = mSpiMaster.WriteReadDMA(element.src, element.dest, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            case RequestType::ReadDMA:
                result = mSpiMaster.ReadDMA(element.dest, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            case RequestType::WriteInterrupt:
                result = mSpiMaster.WriteInterrupt(element.src, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            case RequestType::WriteReadInterrupt:
                result = mSpiMaster.WriteReadInterrupt(element.src, element.dest, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            case RequestType::ReadInterrupt:
                result = mSpiMaster.ReadInterrupt(element.dest, element.length, [this]() { this->DataRequestHandler(); });
                EXPECT(result);
                break;
            default: EXPECT(false); break;  // Invalid request type
        };
    }
    else
    {
        mBusy = false;
    }

    UNUSED(result);
}
