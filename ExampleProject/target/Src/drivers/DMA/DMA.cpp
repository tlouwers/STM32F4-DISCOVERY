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
    mHalfBufferInterrupt(HalfBufferInterrupt::Enabled)
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

    HAL_DMA_DeInit(&mHandle);
}

/**
 * \brief   Configure the DMA object.
 * \param   channel             The DMA channel to configure for.
 * \param   direction           The direction of the DMA to use.
 * \param   bufferMode          The buffer mode to use.
 * \param   width               Memory data width to use. Default Byte size.
 * \param   priority            DMA priority. Default Low.
 * \param   halfBufferInterrupt Flag, indicating half buffer interrupt is to be used or not. Default true.
 * \returns True if the DMA object could be configured, else false.
 * \note    Peripheral data width is fixed at Byte size.
 */
bool DMA::Configure(Channel channel, Direction direction, BufferMode bufferMode, DataWidth width /* = DataWidth::Byte */, Priority priority /* = Priority::Low */, HalfBufferInterrupt halfBufferInterrupt /* = HalfBufferInterrupt::Enabled */)
{
    mHalfBufferInterrupt = halfBufferInterrupt;

    if (__HAL_RCC_DMA1_IS_CLK_DISABLED()) { __HAL_RCC_DMA1_CLK_ENABLE(); }
    if (__HAL_RCC_DMA2_IS_CLK_DISABLED()) { __HAL_RCC_DMA2_CLK_ENABLE(); }

    mHandle.Init.Channel             = GetChannel(channel);
    mHandle.Init.Direction           = GetDirection(direction);
    mHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    mHandle.Init.MemInc              = DMA_MINC_ENABLE;
    mHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;     // Fixed ay Byte size.
    mHandle.Init.MemDataAlignment    = GetDataWidth(width);
    mHandle.Init.Mode                = (bufferMode == DMA::BufferMode::Circular) ? DMA_CIRCULAR : DMA_NORMAL;
    mHandle.Init.Priority            = GetPriority(priority);
    mHandle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&mHandle) == HAL_OK)
    {
        ConnectInternalCallback(mStream);
        EnableInterrupt(mStream, 0, 0);
        return true;
    }

    return false;
}

/**
 * \brief   Method to 'link' a DMA object with a peripheral.
 * \param   parent  The parent to link with, this is the handle of the peripheral.
 * \param   handle  The DMA handle of the peripheral which is 'swapped' with the configured DMA object.
 * \returns True if the DMA object could be linked, else false.
 */
bool DMA::Link(const void* parent, DMA_HandleTypeDef*& handle)
{
    if (parent == nullptr) { return false; }

    mHandle.Parent = const_cast<void*>(parent);
    handle         = &mHandle;

    if (mHalfBufferInterrupt == HalfBufferInterrupt::Disabled)
    {
        __HAL_DMA_DISABLE_IT(handle, DMA_IT_HT);
    }

    return true;
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
uint32_t DMA::GetDirection(Direction direction)
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
 * \brief   Get the DMA data width as register value.
 * \param   width   The data width to get the register value for.
 * \returns The data width as register value.
 */
uint32_t DMA::GetDataWidth(DataWidth width)
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
 * \brief   Connect the internal DMA interrupt handling to the DMA object instance.
 * \param   stream  The DMA stream to connect the callback administration for.
 */
void DMA::ConnectInternalCallback(Stream stream)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: dma1Callbacks[0] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream1: dma1Callbacks[1] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream2: dma1Callbacks[2] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream3: dma1Callbacks[3] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream4: dma1Callbacks[4] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream5: dma1Callbacks[5] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream6: dma1Callbacks[6] = [this]() { this->Callback(); }; break;
        case Stream::Dma1_Stream7: dma1Callbacks[7] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream0: dma2Callbacks[0] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream1: dma2Callbacks[1] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream2: dma2Callbacks[2] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream3: dma2Callbacks[3] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream4: dma2Callbacks[4] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream5: dma2Callbacks[5] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream6: dma2Callbacks[6] = [this]() { this->Callback(); }; break;
        case Stream::Dma2_Stream7: dma2Callbacks[7] = [this]() { this->Callback(); }; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
}

/**
 * \brief   Enable the DMA interrupts with given parameters.
 * \param   stream      The DMA stream to configure the interrupts for.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void DMA::EnableInterrupt(Stream stream, uint32_t preemptPrio, uint32_t subPrio)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: { SetIRQn(DMA1_Stream0_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream1: { SetIRQn(DMA1_Stream1_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream2: { SetIRQn(DMA1_Stream2_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream3: { SetIRQn(DMA1_Stream3_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream4: { SetIRQn(DMA1_Stream4_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream5: { SetIRQn(DMA1_Stream5_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream6: { SetIRQn(DMA1_Stream6_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma1_Stream7: { SetIRQn(DMA1_Stream7_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream0: { SetIRQn(DMA2_Stream0_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream1: { SetIRQn(DMA2_Stream1_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream2: { SetIRQn(DMA2_Stream2_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream3: { SetIRQn(DMA2_Stream3_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream4: { SetIRQn(DMA2_Stream4_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream5: { SetIRQn(DMA2_Stream5_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream6: { SetIRQn(DMA2_Stream6_IRQn, preemptPrio, subPrio); } break;
        case Stream::Dma2_Stream7: { SetIRQn(DMA2_Stream7_IRQn, preemptPrio, subPrio); } break;
        default: ASSERT(false); while(1) { __NOP(); } break; // Impossible selection
    }
}

/**
 * \brief   Disable the DMA interrupts for the given stream.
 * \param   stream  The DMA stream to disable the interrupts for.
 */
void DMA::DisableInterrupt(Stream stream)
{
    switch (stream)
    {
        case Stream::Dma1_Stream0: { HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn); } break;
        case Stream::Dma1_Stream1: { HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn); } break;
        case Stream::Dma1_Stream2: { HAL_NVIC_DisableIRQ(DMA1_Stream2_IRQn); } break;
        case Stream::Dma1_Stream3: { HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn); } break;
        case Stream::Dma1_Stream4: { HAL_NVIC_DisableIRQ(DMA1_Stream4_IRQn); } break;
        case Stream::Dma1_Stream5: { HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn); } break;
        case Stream::Dma1_Stream6: { HAL_NVIC_DisableIRQ(DMA1_Stream6_IRQn); } break;
        case Stream::Dma1_Stream7: { HAL_NVIC_DisableIRQ(DMA1_Stream7_IRQn); } break;
        case Stream::Dma2_Stream0: { HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn); } break;
        case Stream::Dma2_Stream1: { HAL_NVIC_DisableIRQ(DMA2_Stream1_IRQn); } break;
        case Stream::Dma2_Stream2: { HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn); } break;
        case Stream::Dma2_Stream3: { HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn); } break;
        case Stream::Dma2_Stream4: { HAL_NVIC_DisableIRQ(DMA2_Stream4_IRQn); } break;
        case Stream::Dma2_Stream5: { HAL_NVIC_DisableIRQ(DMA2_Stream5_IRQn); } break;
        case Stream::Dma2_Stream6: { HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn); } break;
        case Stream::Dma2_Stream7: { HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn); } break;
        default: ASSERT(false); while(1) { __NOP(); } break; // Impossible selection
    }
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
void DMA::Callback() const
{
    HAL_DMA_IRQHandler(const_cast<DMA_HandleTypeDef*>(&mHandle));
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
