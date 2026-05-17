/**
 * \file    DMA.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   DMA
 *
 * \brief   DMA utility class, intended for peripherals only.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/DMA
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    09-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <functional>
#include "drivers/DMA/DMA.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static std::function<void()> dma1Callbacks[8] {};
static std::function<void()> dma2Callbacks[8] {};


/************************************************************************/
/* Public methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor.
 * \param   stream  Stream to use for the DMA object.
 */
DMA::DMA(Stream stream) :
    mStream(stream),
    mDirection(Direction::MemoryToPeripheral),
    mHalfBufferInterrupt(HalfBufferInterrupt::Enabled),
    mConfigured(false)
{
    ASSERT(mHandle.Instance == nullptr);

    mHandle.Instance = GetInstance(stream);
}

/**
 * \brief   Destructor.
 */
DMA::~DMA()
{
    DisableInterrupt(mStream);
    DisconnectInternalCallback(mStream);

    HAL_DMA_DeInit(&mHandle);
}

/**
 * \brief   Configure the DMA object.
 * \param   channel             The DMA channel to configure for.
 * \param   direction           The direction of the DMA to use.
 * \param   bufferMode          The buffer mode to use.
 * \param   memWidth            Memory-side data width. Default Byte.
 * \param   priority            DMA priority. Default Low.
 * \param   halfBufferInterrupt Flag, indicating half buffer interrupt is to be used or not. Default enabled.
 * \param   periphWidth         Peripheral-side data width. Default Byte.
 *                              Must match the peripheral's frame size (e.g.
 *                              HalfWord for 16-bit I2S / SPI / ADC).
 * \param   preemptPrio         NVIC pre-emption priority for the stream IRQ.
 *                              Default 0. Under FreeRTOS this must be
 *                              numerically >= configMAX_SYSCALL_INTERRUPT_PRIORITY
 *                              for any ISR that calls xQueue...FromISR.
 * \param   subPrio             NVIC sub-priority for the stream IRQ. Default 0.
 * \param   fifo                Optional FIFO / burst configuration. Default
 *                              (FIFO off, single bursts) is the previous
 *                              hardcoded behaviour, so existing call sites
 *                              are unaffected.
 * \returns True if the DMA object could be configured, else false.
 * \note    AN4031 §2: FIFO mode + bursts is where AHB throughput is won
 *          (notably SPI-DMA on the F407). FIFO mode is also mandatory when
 *          the memory and peripheral data widths differ. When the FIFO is
 *          disabled the bursts are forced to single here (direct mode,
 *          RM0090 §10.3.11) so an invalid combination cannot reach
 *          HAL_DMA_Init. The caller is responsible for a threshold/burst
 *          pairing valid for the chosen widths (AN4031 §2.2).
 * \note    ES0182 §2.1.13: an RCC peripheral-enable needs a short delay
 *          before the peripheral is accessible; the __HAL_RCC_DMAx_CLK_ENABLE
 *          macros perform the dummy-read so no explicit barrier is needed here.
 * \note    ES0182 §2.8.x: concurrent DMA2 AHB/APB accesses can corrupt data.
 *          A Direction::MemoryToMemory transfer scheduled on a DMA2 stream
 *          while another DMA2 stream is active is the classic trigger; keep
 *          mem-to-mem on DMA2 isolated from concurrent DMA2 peripheral traffic.
 */
bool DMA::Configure(Channel channel, Direction direction, BufferMode bufferMode,
                    DataWidth memWidth /* = DataWidth::Byte */,
                    Priority priority /* = Priority::Low */,
                    HalfBufferInterrupt halfBufferInterrupt /* = HalfBufferInterrupt::Enabled */,
                    DataWidth periphWidth /* = DataWidth::Byte */,
                    uint32_t preemptPrio /* = 0 */,
                    uint32_t subPrio /* = 0 */,
                    const Fifo& fifo /* = Fifo() */)
{
    mConfigured          = false;
    mDirection           = direction;
    mHalfBufferInterrupt = halfBufferInterrupt;

    if (__HAL_RCC_DMA1_IS_CLK_DISABLED()) { __HAL_RCC_DMA1_CLK_ENABLE(); }
    if (__HAL_RCC_DMA2_IS_CLK_DISABLED()) { __HAL_RCC_DMA2_CLK_ENABLE(); }

    mHandle.Init.Channel             = GetChannel(channel);
    mHandle.Init.Direction           = GetHalDirection(direction);
    // For mem-to-mem the peripheral port (PAR) holds the source buffer, so it
    // must increment too; for peripheral transfers it stays fixed on the FIFO/DR.
    mHandle.Init.PeriphInc           = (direction == Direction::MemoryToMemory) ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
    mHandle.Init.MemInc              = DMA_MINC_ENABLE;
    mHandle.Init.PeriphDataAlignment = GetPeriphDataAlign(periphWidth);
    mHandle.Init.MemDataAlignment    = GetMemDataAlign(memWidth);
    mHandle.Init.Mode                = (bufferMode == DMA::BufferMode::Circular) ? DMA_CIRCULAR : DMA_NORMAL;
    mHandle.Init.Priority            = GetPriority(priority);

    if (fifo.mMode == FifoMode::Enable)
    {
        mHandle.Init.FIFOMode      = DMA_FIFOMODE_ENABLE;
        mHandle.Init.FIFOThreshold = GetFifoThreshold(fifo.mThreshold);
        mHandle.Init.MemBurst      = GetMemBurst(fifo.mMemBurst);
        mHandle.Init.PeriphBurst   = GetPeriphBurst(fifo.mPeriphBurst);
    }
    else
    {
        // Direct mode: a disabled FIFO mandates single beats (RM0090
        // §10.3.11). Force them so a stray burst on an off-FIFO call
        // cannot reach HAL_DMA_Init with an invalid combination.
        mHandle.Init.FIFOMode      = DMA_FIFOMODE_DISABLE;
        mHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        mHandle.Init.MemBurst      = DMA_MBURST_SINGLE;
        mHandle.Init.PeriphBurst   = DMA_PBURST_SINGLE;
    }

    if (HAL_DMA_Init(&mHandle) == HAL_OK)
    {
        ConnectInternalCallback(mStream);
        EnableInterrupt(mStream, preemptPrio, subPrio);
        mConfigured = true;
        return true;
    }

    return false;
}

/**
 * \brief   Indicates whether Configure() has successfully run on this object.
 * \returns True if the DMA stream has been configured, else false.
 */
bool DMA::IsConfigured() const
{
    return mConfigured;
}

/**
 * \brief   Returns the direction the stream was configured for.
 * \returns The Direction passed to Configure(); value is meaningful only
 *          when IsConfigured() returns true.
 */
DMA::Direction DMA::GetDirection() const
{
    return mDirection;
}

/**
 * \brief   Access the underlying HAL DMA handle.
 * \details Used by peripheral drivers in their LinkDma() implementation to
 *          wire the stream into the peripheral's hdmatx / hdmarx slot via
 *          __HAL_LINKDMA. Not intended for application code.
 * \returns Pointer to the underlying DMA_HandleTypeDef. Never nullptr.
 */
DMA_HandleTypeDef* DMA::Handle()
{
    return &mHandle;
}

/**
 * \brief   Indicates whether the user requested the half-transfer interrupt
 *          to remain enabled after Configure().
 * \returns True if the half-transfer interrupt should be delivered, else false.
 */
bool DMA::IsHalfBufferInterruptEnabled() const
{
    return mHalfBufferInterrupt == HalfBufferInterrupt::Enabled;
}

/**
 * \brief   Re-apply the caller's HalfBufferInterrupt preference to the stream.
 * \details HAL_xxx_Receive_DMA (and HAL_DAC_Start_DMA) install a half-complete
 *          callback and call HAL_DMA_Start_IT, which unconditionally re-enables
 *          DMA_IT_HT in the stream CR whenever XferHalfCpltCallback is non-NULL
 *          - silently undoing the HalfBufferInterrupt::Disabled selection made
 *          at Configure() time. Peripheral drivers call this after a successful
 *          receive-start so the user's setting wins.
 */
void DMA::EnforceHalfBufferInterruptSetting()
{
    if (!IsHalfBufferInterruptEnabled())
    {
        __HAL_DMA_DISABLE_IT(&mHandle, DMA_IT_HT);
    }
}


/************************************************************************/
/* Private methods                                                      */
/************************************************************************/
/**
 * \brief   Get the DMA instance belonging to the given stream.
 * \param   stream  The stream to get the DMA instance for.
 * \returns The DMA instance belonging to the given stream.
 */
DMA_Stream_TypeDef* DMA::GetInstance(Stream stream)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: return DMA1_Stream0; break;
        case Stream::Dma1_Stream1: return DMA1_Stream1; break;
        case Stream::Dma1_Stream2: return DMA1_Stream2; break;
        case Stream::Dma1_Stream3: return DMA1_Stream3; break;
        case Stream::Dma1_Stream4: return DMA1_Stream4; break;
        case Stream::Dma1_Stream5: return DMA1_Stream5; break;
        case Stream::Dma1_Stream6: return DMA1_Stream6; break;
        case Stream::Dma1_Stream7: return DMA1_Stream7; break;
        case Stream::Dma2_Stream0: return DMA2_Stream0; break;
        case Stream::Dma2_Stream1: return DMA2_Stream1; break;
        case Stream::Dma2_Stream2: return DMA2_Stream2; break;
        case Stream::Dma2_Stream3: return DMA2_Stream3; break;
        case Stream::Dma2_Stream4: return DMA2_Stream4; break;
        case Stream::Dma2_Stream5: return DMA2_Stream5; break;
        case Stream::Dma2_Stream6: return DMA2_Stream6; break;
        case Stream::Dma2_Stream7: return DMA2_Stream7; break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA1_Stream0; break;    // Impossible selection
    }
}

/**
 * \brief   Get the DMA channel address.
 * \param   channel The DMA channel to get the address from.
 * \returns The DMA channel address.
 */
uint32_t DMA::GetChannel(Channel channel)
{
    switch (channel)
    {
        case Channel::Channel0: return DMA_CHANNEL_0; break;
        case Channel::Channel1: return DMA_CHANNEL_1; break;
        case Channel::Channel2: return DMA_CHANNEL_2; break;
        case Channel::Channel3: return DMA_CHANNEL_3; break;
        case Channel::Channel4: return DMA_CHANNEL_4; break;
        case Channel::Channel5: return DMA_CHANNEL_5; break;
        case Channel::Channel6: return DMA_CHANNEL_6; break;
        case Channel::Channel7: return DMA_CHANNEL_7; break;
#if defined (DMA_SxCR_CHSEL_3)
        case Channel::Channel8:  return DMA_CHANNEL_8;  break;
        case Channel::Channel9:  return DMA_CHANNEL_9;  break;
        case Channel::Channel10: return DMA_CHANNEL_10; break;
        case Channel::Channel11: return DMA_CHANNEL_11; break;
        case Channel::Channel12: return DMA_CHANNEL_12; break;
        case Channel::Channel13: return DMA_CHANNEL_13; break;
        case Channel::Channel14: return DMA_CHANNEL_14; break;
        case Channel::Channel15: return DMA_CHANNEL_15; break;
#endif
        default: ASSERT(false); while(1) { __NOP(); } return DMA_CHANNEL_0; break;    // Impossible selection
    }
}

/**
 * \brief   Get the DMA direction as register value.
 * \param   direction   The direction to get the register value for.
 * \returns The direction as register value.
 */
uint32_t DMA::GetHalDirection(Direction direction)
{
    switch (direction)
    {
        case Direction::MemoryToPeripheral: return DMA_MEMORY_TO_PERIPH; break;
        case Direction::PeripheralToMemory: return DMA_PERIPH_TO_MEMORY; break;
        case Direction::MemoryToMemory:     return DMA_MEMORY_TO_MEMORY; break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_MEMORY_TO_MEMORY; break;    // Impossible selection
    }
}

/**
 * \brief   Get the DMA memory-side data alignment as register value.
 * \param   width   The data width to get the register value for.
 * \returns The memory data alignment as register value.
 * \note    Returns DMA_MDATAALIGN_* (MSIZE field), not DMA_PDATAALIGN_*; the
 *          two macro families occupy different bits of the stream CR.
 */
uint32_t DMA::GetMemDataAlign(DataWidth width)
{
    switch (width)
    {
        case DataWidth::Byte:     return DMA_MDATAALIGN_BYTE;     break;
        case DataWidth::HalfWord: return DMA_MDATAALIGN_HALFWORD; break;
        case DataWidth::Word:     return DMA_MDATAALIGN_WORD;     break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_MDATAALIGN_BYTE; break;    // Impossible selection
    }
}

/**
 * \brief   Get the DMA peripheral-side data alignment as register value.
 * \param   width   The data width to get the register value for.
 * \returns The peripheral data alignment as register value.
 * \note    Returns DMA_PDATAALIGN_* (PSIZE field), not DMA_MDATAALIGN_*; the
 *          two macro families occupy different bits of the stream CR.
 */
uint32_t DMA::GetPeriphDataAlign(DataWidth width)
{
    switch (width)
    {
        case DataWidth::Byte:     return DMA_PDATAALIGN_BYTE;     break;
        case DataWidth::HalfWord: return DMA_PDATAALIGN_HALFWORD; break;
        case DataWidth::Word:     return DMA_PDATAALIGN_WORD;     break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_PDATAALIGN_BYTE; break;    // Impossible selection
    }
}

/**
 * \brief   Get the DMA priority as register value.
 * \param   priority    The priority to get the register value for.
 * \returns The priority as register value.
 */
uint32_t DMA::GetPriority(Priority priority)
{
    switch (priority)
    {
        case Priority::Low:      return DMA_PRIORITY_LOW;       break;
        case Priority::Medium:   return DMA_PRIORITY_MEDIUM;    break;
        case Priority::High:     return DMA_PRIORITY_HIGH;      break;
        case Priority::VeryHigh: return DMA_PRIORITY_VERY_HIGH; break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_PRIORITY_LOW; break;    // Impossible selection
    }
}

/**
 * \brief   Get the FIFO threshold as register value.
 * \param   threshold   The FIFO threshold to get the register value for.
 * \returns The FIFO threshold as register value.
 */
uint32_t DMA::GetFifoThreshold(FifoThreshold threshold)
{
    switch (threshold)
    {
        case FifoThreshold::Quarter:       return DMA_FIFO_THRESHOLD_1QUARTERFULL;  break;
        case FifoThreshold::Half:          return DMA_FIFO_THRESHOLD_HALFFULL;      break;
        case FifoThreshold::ThreeQuarters: return DMA_FIFO_THRESHOLD_3QUARTERSFULL; break;
        case FifoThreshold::Full:          return DMA_FIFO_THRESHOLD_FULL;          break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_FIFO_THRESHOLD_FULL; break;    // Impossible selection
    }
}

/**
 * \brief   Get the memory-side burst as register value.
 * \param   burst   The memory burst to get the register value for.
 * \returns The memory burst as register value.
 */
uint32_t DMA::GetMemBurst(Burst burst)
{
    switch (burst)
    {
        case Burst::Single:      return DMA_MBURST_SINGLE; break;
        case Burst::Increment4:  return DMA_MBURST_INC4;   break;
        case Burst::Increment8:  return DMA_MBURST_INC8;   break;
        case Burst::Increment16: return DMA_MBURST_INC16;  break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_MBURST_SINGLE; break;    // Impossible selection
    }
}

/**
 * \brief   Get the peripheral-side burst as register value.
 * \param   burst   The peripheral burst to get the register value for.
 * \returns The peripheral burst as register value.
 */
uint32_t DMA::GetPeriphBurst(Burst burst)
{
    switch (burst)
    {
        case Burst::Single:      return DMA_PBURST_SINGLE; break;
        case Burst::Increment4:  return DMA_PBURST_INC4;   break;
        case Burst::Increment8:  return DMA_PBURST_INC8;   break;
        case Burst::Increment16: return DMA_PBURST_INC16;  break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA_PBURST_SINGLE; break;    // Impossible selection
    }
}

/**
 * \brief   Get the IRQn for the given DMA stream.
 * \param   stream  The DMA stream to get the IRQn for.
 * \returns The IRQn corresponding to the DMA stream.
 */
IRQn_Type DMA::GetIRQn(Stream stream)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: return DMA1_Stream0_IRQn; break;
        case Stream::Dma1_Stream1: return DMA1_Stream1_IRQn; break;
        case Stream::Dma1_Stream2: return DMA1_Stream2_IRQn; break;
        case Stream::Dma1_Stream3: return DMA1_Stream3_IRQn; break;
        case Stream::Dma1_Stream4: return DMA1_Stream4_IRQn; break;
        case Stream::Dma1_Stream5: return DMA1_Stream5_IRQn; break;
        case Stream::Dma1_Stream6: return DMA1_Stream6_IRQn; break;
        case Stream::Dma1_Stream7: return DMA1_Stream7_IRQn; break;
        case Stream::Dma2_Stream0: return DMA2_Stream0_IRQn; break;
        case Stream::Dma2_Stream1: return DMA2_Stream1_IRQn; break;
        case Stream::Dma2_Stream2: return DMA2_Stream2_IRQn; break;
        case Stream::Dma2_Stream3: return DMA2_Stream3_IRQn; break;
        case Stream::Dma2_Stream4: return DMA2_Stream4_IRQn; break;
        case Stream::Dma2_Stream5: return DMA2_Stream5_IRQn; break;
        case Stream::Dma2_Stream6: return DMA2_Stream6_IRQn; break;
        case Stream::Dma2_Stream7: return DMA2_Stream7_IRQn; break;
        default: ASSERT(false); while(1) { __NOP(); } return DMA1_Stream0_IRQn; break;    // Impossible selection
    }
}

/**
 * \brief   Get a reference to the static callback slot for the given stream.
 * \param   stream  The DMA stream whose slot to access.
 * \returns Reference to the std::function slot for that stream.
 */
std::function<void()>& DMA::GetCallbackSlot(Stream stream)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: return dma1Callbacks[0]; break;
        case Stream::Dma1_Stream1: return dma1Callbacks[1]; break;
        case Stream::Dma1_Stream2: return dma1Callbacks[2]; break;
        case Stream::Dma1_Stream3: return dma1Callbacks[3]; break;
        case Stream::Dma1_Stream4: return dma1Callbacks[4]; break;
        case Stream::Dma1_Stream5: return dma1Callbacks[5]; break;
        case Stream::Dma1_Stream6: return dma1Callbacks[6]; break;
        case Stream::Dma1_Stream7: return dma1Callbacks[7]; break;
        case Stream::Dma2_Stream0: return dma2Callbacks[0]; break;
        case Stream::Dma2_Stream1: return dma2Callbacks[1]; break;
        case Stream::Dma2_Stream2: return dma2Callbacks[2]; break;
        case Stream::Dma2_Stream3: return dma2Callbacks[3]; break;
        case Stream::Dma2_Stream4: return dma2Callbacks[4]; break;
        case Stream::Dma2_Stream5: return dma2Callbacks[5]; break;
        case Stream::Dma2_Stream6: return dma2Callbacks[6]; break;
        case Stream::Dma2_Stream7: return dma2Callbacks[7]; break;
        default: ASSERT(false); while(1) { __NOP(); } return dma1Callbacks[0]; break;    // Impossible selection
    }
}

/**
 * \brief   Connect the internal DMA interrupt handling to the DMA object instance.
 * \param   stream  The DMA stream to connect the callback administration for.
 */
void DMA::ConnectInternalCallback(Stream stream)
{
    GetCallbackSlot(stream) = [this]() { this->Callback(); };
}

/**
 * \brief   Disconnect the internal DMA interrupt handling for this object.
 * \param   stream  The DMA stream to disconnect.
 * \note    Called from the destructor to drop the lambda's captured 'this',
 *          so a stray IRQ cannot dispatch into a destroyed object.
 */
void DMA::DisconnectInternalCallback(Stream stream)
{
    GetCallbackSlot(stream) = nullptr;
}

/**
 * \brief   Enable the DMA interrupts with given parameters.
 * \param   stream      The DMA stream to configure the interrupts for.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void DMA::EnableInterrupt(Stream stream, uint32_t preemptPrio, uint32_t subPrio)
{
    SetIRQn(GetIRQn(stream), preemptPrio, subPrio);
}

/**
 * \brief   Disable the DMA interrupts for the given stream.
 * \param   stream  The DMA stream to disable the interrupts for.
 */
void DMA::DisableInterrupt(Stream stream)
{
    HAL_NVIC_DisableIRQ(GetIRQn(stream));
}

/**
 * \brief   Lower level configuration for the DMA interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void DMA::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
}

/**
 * \brief   Internal class callback which connects the DMA HAL callbacks to this object.
 */
void DMA::Callback()
{
    HAL_DMA_IRQHandler(&mHandle);
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: route DMA1 Stream0 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream0_IRQHandler(void)
{
    if (dma1Callbacks[0]) { dma1Callbacks[0](); }
}

/**
 * \brief   ISR: route DMA1 Stream1 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream1_IRQHandler(void)
{
    if (dma1Callbacks[1]) { dma1Callbacks[1](); }
}

/**
 * \brief   ISR: route DMA1 Stream2 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream2_IRQHandler(void)
{
    if (dma1Callbacks[2]) { dma1Callbacks[2](); }
}

/**
 * \brief   ISR: route DMA1 Stream3 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream3_IRQHandler(void)
{
    if (dma1Callbacks[3]) { dma1Callbacks[3](); }
}

/**
 * \brief   ISR: route DMA1 Stream4 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream4_IRQHandler(void)
{
    if (dma1Callbacks[4]) { dma1Callbacks[4](); }
}

/**
 * \brief   ISR: route DMA1 Stream5 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream5_IRQHandler(void)
{
    if (dma1Callbacks[5]) { dma1Callbacks[5](); }
}

/**
 * \brief   ISR: route DMA1 Stream6 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream6_IRQHandler(void)
{
    if (dma1Callbacks[6]) { dma1Callbacks[6](); }
}

/**
 * \brief   ISR: route DMA1 Stream7 interrupts to 'Callback'.
 */
extern "C" void DMA1_Stream7_IRQHandler(void)
{
    if (dma1Callbacks[7]) { dma1Callbacks[7](); }
}

/**
 * \brief   ISR: route DMA2 Stream0 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream0_IRQHandler(void)
{
    if (dma2Callbacks[0]) { dma2Callbacks[0](); }
}

/**
 * \brief   ISR: route DMA2 Stream1 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream1_IRQHandler(void)
{
    if (dma2Callbacks[1]) { dma2Callbacks[1](); }
}

/**
 * \brief   ISR: route DMA2 Stream2 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream2_IRQHandler(void)
{
    if (dma2Callbacks[2]) { dma2Callbacks[2](); }
}

/**
 * \brief   ISR: route DMA2 Stream3 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream3_IRQHandler(void)
{
    if (dma2Callbacks[3]) { dma2Callbacks[3](); }
}

/**
 * \brief   ISR: route DMA2 Stream4 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream4_IRQHandler(void)
{
    if (dma2Callbacks[4]) { dma2Callbacks[4](); }
}

/**
 * \brief   ISR: route DMA2 Stream5 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream5_IRQHandler(void)
{
    if (dma2Callbacks[5]) { dma2Callbacks[5](); }
}

/**
 * \brief   ISR: route DMA2 Stream6 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream6_IRQHandler(void)
{
    if (dma2Callbacks[6]) { dma2Callbacks[6](); }
}

/**
 * \brief   ISR: route DMA2 Stream7 interrupts to 'Callback'.
 */
extern "C" void DMA2_Stream7_IRQHandler(void)
{
    if (dma2Callbacks[7]) { dma2Callbacks[7](); }
}
