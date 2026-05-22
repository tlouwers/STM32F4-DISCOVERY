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
// The driver installs these C-linkage ISR entry points; the test invokes one
// directly to prove the stream vector dispatches into the driver's Callback().
extern "C" void DMA1_Stream0_IRQHandler(void);


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


} // namespace
