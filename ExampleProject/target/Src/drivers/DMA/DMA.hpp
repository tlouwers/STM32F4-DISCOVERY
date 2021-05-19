/**
 * \file    DMA.hpp
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

#ifndef DMA_HPP_
#define DMA_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class DMA final
{
public:
    /**
     * \enum    Stream
     * \brief   Available DMA streams.
     */
    enum class Stream : uint8_t
    {
        Dma1_Stream0,
        Dma1_Stream1,
        Dma1_Stream2,
        Dma1_Stream3,
        Dma1_Stream4,
        Dma1_Stream5,
        Dma1_Stream6,
        Dma1_Stream7,
        Dma2_Stream0,
        Dma2_Stream1,
        Dma2_Stream2,
        Dma2_Stream3,
        Dma2_Stream4,
        Dma2_Stream5,
        Dma2_Stream6,
        Dma2_Stream7
    };

    /**
     * \enum    Channel
     * \brief   Available DMA channels.
     */
    enum class Channel : uint8_t
    {
        Channel0,
        Channel1,
        Channel2,
        Channel3,
        Channel4,
        Channel5,
        Channel6,
        Channel7,
#if defined (DMA_SxCR_CHSEL_3)
        Channel8,
        Channel9,
        Channel10,
        Channel11,
        Channel12,
        Channel13,
        Channel14,
        Channel15
#endif
    };

    /**
     * \enum    Direction
     * \brief   Available DMA directions.
     */
    enum class Direction : uint8_t
    {
        MemoryToPeripheral,
        PeripheralToMemory,
        MemoryToMemory
    };

    /**
     * \enum    DataWidth
     * \brief   Available data widths.
     */
    enum class DataWidth : uint8_t
    {
        Byte,
        HalfWord,
        Word
    };

    /**
     * \enum    BufferMode
     * \brief   Available DMA buffer modes.
     */
    enum class BufferMode : bool
    {
        Normal,
        Circular
    };

    /**
     * \enum    Priority
     * \brief   Available DMA priorities.
     */
    enum class Priority : uint8_t
    {
        Low,
        Medium,
        High,
        VeryHigh
    };

    /**
     * \enum    HalfBufferInterrupt
     * \brief   Indicator if the half buffer interrupt is to be used or not.
     */
    enum class HalfBufferInterrupt : bool
    {
        Enabled,
        Disabled
    };


    explicit DMA(Stream stream);
    ~DMA();

    bool Configure(Channel channel, Direction direction, BufferMode bufferMode, DataWidth width = DataWidth::Byte, Priority priority = Priority::Low, HalfBufferInterrupt halfBufferInterrupt = HalfBufferInterrupt::Enabled);
    bool Link(const void* parent, DMA_HandleTypeDef*& handle);

private:
    DMA_HandleTypeDef   mHandle = {};
    Stream              mStream;
    HalfBufferInterrupt mHalfBufferInterrupt;

    DMA_Stream_TypeDef* GetInstance(Stream stream);
    uint32_t GetChannel(Channel channel);
    uint32_t GetDirection(Direction direction);
    uint32_t GetDataWidth(DataWidth width);
    uint32_t GetPriority(Priority priority);

    void ConnectInternalCallback(Stream stream);
    void EnableInterrupt(Stream stream, uint32_t preemptPrio, uint32_t subPrio);
    void DisableInterrupt(Stream stream);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);

    void Callback() const;
};


#endif  // DMA_HPP_
