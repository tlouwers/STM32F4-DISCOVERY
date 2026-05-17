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
#include <functional>
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

    /**
     * \enum    FifoMode
     * \brief   DMA FIFO mode. Disable = direct mode (default, prior
     *          behaviour); Enable = FIFO mode, required for bursts and
     *          for differing mem/periph data widths (AN4031 §2.2).
     */
    enum class FifoMode : bool
    {
        Disable,
        Enable
    };

    /**
     * \enum    FifoThreshold
     * \brief   FIFO fill level that triggers a memory burst. Only used
     *          when FifoMode::Enable.
     */
    enum class FifoThreshold : uint8_t
    {
        Quarter,
        Half,
        ThreeQuarters,
        Full
    };

    /**
     * \enum    Burst
     * \brief   Incremental AHB burst beat count for the memory /
     *          peripheral side. Single = no burst (default). Only used
     *          when FifoMode::Enable.
     */
    enum class Burst : uint8_t
    {
        Single,
        Increment4,
        Increment8,
        Increment16
    };

    /**
     * \struct  Fifo
     * \brief   Optional DMA FIFO / burst configuration.
     * \details A default-constructed Fifo (FIFO off, single bursts) is
     *          exactly the previous hardcoded behaviour, so existing
     *          Configure() call sites are unaffected. Enabling the FIFO
     *          with bursts is where the real AHB throughput gain lives
     *          (AN4031 §2), especially for SPI-DMA on the F407. The
     *          caller must pick a threshold/burst combination valid for
     *          the chosen data widths (AN4031 §2.2); HAL_DMA_Init
     *          asserts the gross cases.
     */
    struct Fifo
    {
        /**
         * \brief   Constructor of the DMA Fifo configuration struct.
         * \param   mode        FIFO mode -- default Disable (direct mode),
         *                      preserving the previous fixed behaviour.
         * \param   threshold   FIFO threshold -- only used when mode Enable.
         * \param   memBurst    Memory-side burst -- only used when Enable.
         * \param   periphBurst Peripheral-side burst -- only used when Enable.
         */
        Fifo(FifoMode mode = FifoMode::Disable,
             FifoThreshold threshold = FifoThreshold::Full,
             Burst memBurst = Burst::Single,
             Burst periphBurst = Burst::Single) :
            mMode(mode),
            mThreshold(threshold),
            mMemBurst(memBurst),
            mPeriphBurst(periphBurst)
        { }

        FifoMode      mMode;        ///< FIFO mode.
        FifoThreshold mThreshold;   ///< FIFO threshold (used when mMode Enable).
        Burst         mMemBurst;    ///< Memory-side burst (used when mMode Enable).
        Burst         mPeriphBurst; ///< Peripheral-side burst (used when mMode Enable).
    };


    explicit DMA(Stream stream);
    ~DMA();

    bool Configure(Channel channel, Direction direction, BufferMode bufferMode,
                   DataWidth memWidth = DataWidth::Byte,
                   Priority priority = Priority::Low,
                   HalfBufferInterrupt halfBufferInterrupt = HalfBufferInterrupt::Enabled,
                   DataWidth periphWidth = DataWidth::Byte,
                   uint32_t preemptPrio = 0,
                   uint32_t subPrio = 0,
                   const Fifo& fifo = Fifo());

    bool IsConfigured() const;
    Direction GetDirection() const;
    DMA_HandleTypeDef* Handle();

    bool IsHalfBufferInterruptEnabled() const;
    void EnforceHalfBufferInterruptSetting();

private:
    DMA_HandleTypeDef   mHandle = {};
    Stream              mStream;
    Direction           mDirection;
    HalfBufferInterrupt mHalfBufferInterrupt;
    bool                mConfigured;

    DMA_Stream_TypeDef* GetInstance(Stream stream);
    uint32_t GetChannel(Channel channel);
    uint32_t GetHalDirection(Direction direction);
    uint32_t GetMemDataAlign(DataWidth width);
    uint32_t GetPeriphDataAlign(DataWidth width);
    uint32_t GetPriority(Priority priority);
    uint32_t GetFifoThreshold(FifoThreshold threshold);
    uint32_t GetMemBurst(Burst burst);
    uint32_t GetPeriphBurst(Burst burst);
    IRQn_Type GetIRQn(Stream stream);
    std::function<void()>& GetCallbackSlot(Stream stream);

    void ConnectInternalCallback(Stream stream);
    void DisconnectInternalCallback(Stream stream);
    void EnableInterrupt(Stream stream, uint32_t preemptPrio, uint32_t subPrio);
    void DisableInterrupt(Stream stream);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);

    void Callback();
};


#endif  // DMA_HPP_
