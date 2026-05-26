/**
 * \file    TestDMA.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the DMA driver. Exercises the real driver
 *          (drivers/drivers/DMA) against the fake HAL DMA surface
 *          (tests/Fake/stm32f4xx_hal_dma.{h,cpp}).
 *
 * \details Coverage focuses on the genuine driver logic: the Configure() ->
 *          IsConfigured/GetDirection state machine, the HAL_DMA_Init failure
 *          branch, the half-transfer-interrupt reassert (EnforceHalfBuffer-
 *          InterruptSetting), and the stream IRQ vector -> Callback() ->
 *          HAL_DMA_IRQHandler dispatch path. The register-level HAL behaviour
 *          itself is hardware-only and not unit-tested.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"

// Test subject -- the real DMA driver.
#include "drivers/DMA/DMA.hpp"


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs these C-linkage ISR entry points; the tests invoke them
// directly to prove each stream vector dispatches into the driver's Callback().
extern "C" void DMA1_Stream0_IRQHandler(void);
extern "C" void DMA1_Stream1_IRQHandler(void);
extern "C" void DMA1_Stream2_IRQHandler(void);
extern "C" void DMA1_Stream3_IRQHandler(void);
extern "C" void DMA1_Stream4_IRQHandler(void);
extern "C" void DMA1_Stream5_IRQHandler(void);
extern "C" void DMA1_Stream6_IRQHandler(void);
extern "C" void DMA1_Stream7_IRQHandler(void);
extern "C" void DMA2_Stream0_IRQHandler(void);
extern "C" void DMA2_Stream1_IRQHandler(void);
extern "C" void DMA2_Stream2_IRQHandler(void);
extern "C" void DMA2_Stream3_IRQHandler(void);
extern "C" void DMA2_Stream4_IRQHandler(void);
extern "C" void DMA2_Stream5_IRQHandler(void);
extern "C" void DMA2_Stream6_IRQHandler(void);
extern "C" void DMA2_Stream7_IRQHandler(void);


namespace {


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class DMA_Test : public ::testing::Test
{
protected:
    DMA_Test()
    {
        FakeDMA_Reset();
    }
};


/************************************************************************/
/* Tests                                                                */
/************************************************************************/
TEST_F(DMA_Test, IsConfigured_BeforeConfigure_ReturnsFalse)
{
    DMA subject(DMA::Stream::Dma1_Stream0);
    EXPECT_FALSE(subject.IsConfigured());
}

TEST_F(DMA_Test, Handle_Always_ReturnsNonNull)
{
    DMA subject(DMA::Stream::Dma2_Stream3);
    ASSERT_NE(nullptr, subject.Handle());
    EXPECT_NE(nullptr, subject.Handle()->Instance);
}

TEST_F(DMA_Test, Configure_ValidParams_ReturnsTrueAndIsConfigured)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::MemoryToPeripheral,
                                  DMA::BufferMode::Normal));
    EXPECT_TRUE(subject.IsConfigured());
}

TEST_F(DMA_Test, GetDirection_AfterConfigure_ReturnsConfiguredDirection)
{
    DMA subject(DMA::Stream::Dma1_Stream4);

    ASSERT_TRUE(subject.Configure(DMA::Channel::Channel3,
                                  DMA::Direction::PeripheralToMemory,
                                  DMA::BufferMode::Circular));
    EXPECT_EQ(DMA::Direction::PeripheralToMemory, subject.GetDirection());
}

TEST_F(DMA_Test, Configure_HalInitFails_ReturnsFalseAndNotConfigured)
{
    FakeDMA_SetInitResult(HAL_ERROR);

    DMA subject(DMA::Stream::Dma1_Stream0);

    EXPECT_FALSE(subject.Configure(DMA::Channel::Channel0,
                                   DMA::Direction::MemoryToPeripheral,
                                   DMA::BufferMode::Normal));
    EXPECT_FALSE(subject.IsConfigured());
}

TEST_F(DMA_Test, IsHalfBufferInterruptEnabled_ConfiguredDisabled_ReturnsFalse)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::PeripheralToMemory,
                                  DMA::BufferMode::Normal,
                                  DMA::DataWidth::Byte,
                                  DMA::Priority::Low,
                                  DMA::HalfBufferInterrupt::Disabled));
    EXPECT_FALSE(subject.IsHalfBufferInterruptEnabled());
}

TEST_F(DMA_Test, EnforceHalfBufferInterruptSetting_Disabled_ClearsHtBit)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::PeripheralToMemory,
                                  DMA::BufferMode::Normal,
                                  DMA::DataWidth::Byte,
                                  DMA::Priority::Low,
                                  DMA::HalfBufferInterrupt::Disabled));

    // Simulate HAL_xxx_Receive_DMA having re-enabled the HT interrupt.
    subject.Handle()->Instance->CR |= DMA_IT_HT;

    subject.EnforceHalfBufferInterruptSetting();

    EXPECT_EQ(0U, subject.Handle()->Instance->CR & DMA_IT_HT);
}

TEST_F(DMA_Test, EnforceHalfBufferInterruptSetting_Enabled_LeavesHtBit)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::PeripheralToMemory,
                                  DMA::BufferMode::Normal,
                                  DMA::DataWidth::Byte,
                                  DMA::Priority::Low,
                                  DMA::HalfBufferInterrupt::Enabled));

    subject.Handle()->Instance->CR |= DMA_IT_HT;

    subject.EnforceHalfBufferInterruptSetting();

    // Enabled is the caller's choice -- the bit must be left intact.
    EXPECT_EQ(DMA_IT_HT, subject.Handle()->Instance->CR & DMA_IT_HT);
}

TEST_F(DMA_Test, StreamIRQHandler_AfterConfigure_DispatchesIntoDriverCallback)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::MemoryToPeripheral,
                                  DMA::BufferMode::Normal));
    ASSERT_EQ(0, FakeDMA_IRQHandlerCallCount());

    // Firing the stream vector must route through the registered callback into
    // HAL_DMA_IRQHandler exactly once.
    DMA1_Stream0_IRQHandler();

    EXPECT_EQ(1, FakeDMA_IRQHandlerCallCount());
}

TEST_F(DMA_Test, StreamIRQHandler_AfterDestruction_DoesNotDispatch)
{
    {
        DMA subject(DMA::Stream::Dma1_Stream0);
        ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                      DMA::Direction::MemoryToPeripheral,
                                      DMA::BufferMode::Normal));
    } // subject destroyed -> DisconnectInternalCallback() drops the slot.

    DMA1_Stream0_IRQHandler();

    // No live object: the callback slot is null, so nothing dispatches.
    EXPECT_EQ(0, FakeDMA_IRQHandlerCallCount());
}

// Every stream must map to its own instance/IRQn/callback-slot and its own
// vector must dispatch into the driver -- exercises GetInstance, GetIRQn and
// GetCallbackSlot across all 16 arms plus the 16 stream ISR entry points.
TEST_F(DMA_Test, AllStreams_ConfigureAndFireVector_DispatchesOnce)
{
    struct StreamVector
    {
        DMA::Stream stream;
        void (*vector)(void);
    };

    const StreamVector table[] = {
        { DMA::Stream::Dma1_Stream0, &DMA1_Stream0_IRQHandler },
        { DMA::Stream::Dma1_Stream1, &DMA1_Stream1_IRQHandler },
        { DMA::Stream::Dma1_Stream2, &DMA1_Stream2_IRQHandler },
        { DMA::Stream::Dma1_Stream3, &DMA1_Stream3_IRQHandler },
        { DMA::Stream::Dma1_Stream4, &DMA1_Stream4_IRQHandler },
        { DMA::Stream::Dma1_Stream5, &DMA1_Stream5_IRQHandler },
        { DMA::Stream::Dma1_Stream6, &DMA1_Stream6_IRQHandler },
        { DMA::Stream::Dma1_Stream7, &DMA1_Stream7_IRQHandler },
        { DMA::Stream::Dma2_Stream0, &DMA2_Stream0_IRQHandler },
        { DMA::Stream::Dma2_Stream1, &DMA2_Stream1_IRQHandler },
        { DMA::Stream::Dma2_Stream2, &DMA2_Stream2_IRQHandler },
        { DMA::Stream::Dma2_Stream3, &DMA2_Stream3_IRQHandler },
        { DMA::Stream::Dma2_Stream4, &DMA2_Stream4_IRQHandler },
        { DMA::Stream::Dma2_Stream5, &DMA2_Stream5_IRQHandler },
        { DMA::Stream::Dma2_Stream6, &DMA2_Stream6_IRQHandler },
        { DMA::Stream::Dma2_Stream7, &DMA2_Stream7_IRQHandler },
    };

    for (const StreamVector& sv : table)
    {
        FakeDMA_Reset();

        DMA subject(sv.stream);
        ASSERT_NE(nullptr, subject.Handle()->Instance);
        ASSERT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                      DMA::Direction::MemoryToPeripheral,
                                      DMA::BufferMode::Normal));

        sv.vector();
        EXPECT_EQ(1, FakeDMA_IRQHandlerCallCount());
    }
}

// Every channel selection must translate -- exercises all GetChannel arms.
TEST_F(DMA_Test, Configure_EveryChannel_ReturnsTrue)
{
    const DMA::Channel channels[] = {
        DMA::Channel::Channel0, DMA::Channel::Channel1,
        DMA::Channel::Channel2, DMA::Channel::Channel3,
        DMA::Channel::Channel4, DMA::Channel::Channel5,
        DMA::Channel::Channel6, DMA::Channel::Channel7,
    };

    for (const DMA::Channel channel : channels)
    {
        DMA subject(DMA::Stream::Dma1_Stream0);
        EXPECT_TRUE(subject.Configure(channel,
                                      DMA::Direction::MemoryToPeripheral,
                                      DMA::BufferMode::Normal));
    }
}

// Mem-to-mem is the third direction arm and also flips PeriphInc on.
TEST_F(DMA_Test, Configure_MemoryToMemory_ReturnsTrue)
{
    DMA subject(DMA::Stream::Dma2_Stream0);

    EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::MemoryToMemory,
                                  DMA::BufferMode::Normal));
    EXPECT_EQ(DMA::Direction::MemoryToMemory, subject.GetDirection());
}

// Both mem- and periph-side width translators have a Byte/HalfWord/Word arm.
TEST_F(DMA_Test, Configure_EveryDataWidth_ReturnsTrue)
{
    const DMA::DataWidth widths[] = {
        DMA::DataWidth::Byte, DMA::DataWidth::HalfWord, DMA::DataWidth::Word,
    };

    for (const DMA::DataWidth mem : widths)
    {
        for (const DMA::DataWidth periph : widths)
        {
            DMA subject(DMA::Stream::Dma1_Stream0);
            EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                          DMA::Direction::MemoryToPeripheral,
                                          DMA::BufferMode::Normal,
                                          mem,
                                          DMA::Priority::Low,
                                          DMA::HalfBufferInterrupt::Enabled,
                                          periph));
        }
    }
}

// All four priority arms.
TEST_F(DMA_Test, Configure_EveryPriority_ReturnsTrue)
{
    const DMA::Priority priorities[] = {
        DMA::Priority::Low, DMA::Priority::Medium,
        DMA::Priority::High, DMA::Priority::VeryHigh,
    };

    for (const DMA::Priority priority : priorities)
    {
        DMA subject(DMA::Stream::Dma1_Stream0);
        EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                      DMA::Direction::MemoryToPeripheral,
                                      DMA::BufferMode::Normal,
                                      DMA::DataWidth::Byte,
                                      priority));
    }
}

// Circular buffer mode is the second BufferMode arm.
TEST_F(DMA_Test, Configure_CircularBufferMode_ReturnsTrue)
{
    DMA subject(DMA::Stream::Dma1_Stream0);

    EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                  DMA::Direction::MemoryToPeripheral,
                                  DMA::BufferMode::Circular));
}

// Enabling the FIFO takes the burst/threshold branch -- exercises every arm of
// GetFifoThreshold, GetMemBurst and GetPeriphBurst.
TEST_F(DMA_Test, Configure_FifoEnabled_EveryThresholdAndBurst_ReturnsTrue)
{
    const DMA::FifoThreshold thresholds[] = {
        DMA::FifoThreshold::Quarter, DMA::FifoThreshold::Half,
        DMA::FifoThreshold::ThreeQuarters, DMA::FifoThreshold::Full,
    };
    const DMA::Burst bursts[] = {
        DMA::Burst::Single, DMA::Burst::Increment4,
        DMA::Burst::Increment8, DMA::Burst::Increment16,
    };

    for (int i = 0; i < 4; ++i)
    {
        DMA subject(DMA::Stream::Dma1_Stream0);
        const DMA::Fifo fifo(DMA::FifoMode::Enable, thresholds[i],
                             bursts[i], bursts[i]);

        EXPECT_TRUE(subject.Configure(DMA::Channel::Channel0,
                                      DMA::Direction::MemoryToPeripheral,
                                      DMA::BufferMode::Normal,
                                      DMA::DataWidth::Byte,
                                      DMA::Priority::Low,
                                      DMA::HalfBufferInterrupt::Enabled,
                                      DMA::DataWidth::Byte,
                                      0,
                                      0,
                                      fifo));
    }
}


} // namespace
