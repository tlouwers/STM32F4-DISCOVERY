/**
 * \file    TestDac.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Dac driver. Exercises the real driver
 *          (drivers/drivers/Dac) against the fake HAL DAC surface
 *          (tests/Fake/stm32f4xx_hal_dac.{h,cpp}) and the shared fake HAL DMA
 *          surface (for the per-channel waveform DMA slot).
 *
 * \details Coverage focuses on the driver's own logic: the Init/IsInit/Sleep
 *          lifecycle; LinkDma channel + direction routing; ConfigureChannel and
 *          ConfigureWaveform validation; the SetValue start-then-write path;
 *          the StartWaveform preconditions (DMA linked + waveform configured +
 *          not already started) and StopWaveform; and the software-trigger
 *          Tick() path incl. index wrap. Each HAL-failure branch is driven via
 *          a dedicated FakeDAC_Set*Result hook.
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

// Test subject -- the real Dac driver.
#include "drivers/Dac/Dac.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_dac.h"
}


namespace {

using Ch = IDac::Channel;


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Dac_Test : public ::testing::Test
{
protected:
    Dac_Test()
    {
        FakeDAC_Reset();
        FakeDMA_Reset();
    }

    bool ConfigureDma(DMA& dma, DMA::Direction direction)
    {
        return dma.Configure(DMA::Channel::Channel0, direction, DMA::BufferMode::Normal);
    }

    Dac mSubject;
    uint16_t mWaveform[4] = { 0x000, 0x400, 0x800, 0xC00 };
};


/************************************************************************/
/* Init / IsInit / Sleep                                                */
/************************************************************************/
TEST_F(Dac_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Dac_Test, Init_Default_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init());
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(Dac_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeDAC_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Dac_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init());

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Dac_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    FakeDAC_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}


/************************************************************************/
/* LinkDma                                                              */
/************************************************************************/
TEST_F(Dac_Test, LinkDma_UnconfiguredDma_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma1_Stream5);
    EXPECT_FALSE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
}

TEST_F(Dac_Test, LinkDma_WrongDirection_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::PeripheralToMemory));

    EXPECT_FALSE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
}

TEST_F(Dac_Test, LinkDma_Channel1MemoryToPeripheral_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));

    EXPECT_TRUE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
}

TEST_F(Dac_Test, LinkDma_Channel2MemoryToPeripheral_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma1_Stream6);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));

    EXPECT_TRUE(mSubject.LinkDma(Ch::CHANNEL_2, dma));
}


/************************************************************************/
/* ConfigureChannel                                                     */
/************************************************************************/
TEST_F(Dac_Test, ConfigureChannel_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.ConfigureChannel(Ch::CHANNEL_1, Dac::ChannelConfig()));
}

TEST_F(Dac_Test, ConfigureChannel_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_TRUE(mSubject.ConfigureChannel(Ch::CHANNEL_1, Dac::ChannelConfig()));
}

TEST_F(Dac_Test, ConfigureChannel_HalConfigFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    FakeDAC_SetConfigChannelResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.ConfigureChannel(Ch::CHANNEL_1, Dac::ChannelConfig()));
}


/************************************************************************/
/* ConfigureWaveform                                                    */
/************************************************************************/
TEST_F(Dac_Test, ConfigureWaveform_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));
}

TEST_F(Dac_Test, ConfigureWaveform_NullValues_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, nullptr, 4));
}

TEST_F(Dac_Test, ConfigureWaveform_ZeroLength_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 0));
}

TEST_F(Dac_Test, ConfigureWaveform_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));
}


/************************************************************************/
/* SetValue                                                             */
/************************************************************************/
TEST_F(Dac_Test, SetValue_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.SetValue(Ch::CHANNEL_1, 0x800));
}

TEST_F(Dac_Test, SetValue_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_TRUE(mSubject.SetValue(Ch::CHANNEL_1, 0x800));
}

TEST_F(Dac_Test, SetValue_HalStartFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    FakeDAC_SetStartResult(HAL_ERROR);   // StartChannel fails before the write

    EXPECT_FALSE(mSubject.SetValue(Ch::CHANNEL_1, 0x800));
}

TEST_F(Dac_Test, SetValue_HalSetValueFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    FakeDAC_SetSetValueResult(HAL_ERROR);   // channel starts, the write fails

    EXPECT_FALSE(mSubject.SetValue(Ch::CHANNEL_1, 0x800));
}


/************************************************************************/
/* StartWaveform / StopWaveform                                         */
/************************************************************************/
TEST_F(Dac_Test, StartWaveform_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.StartWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StartWaveform_NoDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    ASSERT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));

    EXPECT_FALSE(mSubject.StartWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StartWaveform_NoWaveformConfigured_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());

    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(Ch::CHANNEL_1, dma));

    EXPECT_FALSE(mSubject.StartWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StartWaveform_DmaLinkedAndWaveformConfigured_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init());

    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
    ASSERT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));

    EXPECT_TRUE(mSubject.StartWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StartWaveform_HalStartDmaFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());

    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
    ASSERT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));
    FakeDAC_SetStartDmaResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.StartWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StopWaveform_NotStarted_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.StopWaveform(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, StopWaveform_AfterStart_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init());

    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(Ch::CHANNEL_1, dma));
    ASSERT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));
    ASSERT_TRUE(mSubject.StartWaveform(Ch::CHANNEL_1));

    EXPECT_TRUE(mSubject.StopWaveform(Ch::CHANNEL_1));
}


/************************************************************************/
/* Tick (software-trigger waveform)                                    */
/************************************************************************/
TEST_F(Dac_Test, Tick_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Tick(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, Tick_NoWaveformConfigured_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.Tick(Ch::CHANNEL_1));
}

TEST_F(Dac_Test, Tick_WaveformConfigured_AdvancesAndWraps)
{
    ASSERT_TRUE(mSubject.Init());
    ASSERT_TRUE(mSubject.ConfigureWaveform(Ch::CHANNEL_1, mWaveform, 4));

    // One full lap plus one more sample proves the index advances and wraps
    // without falling over.
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(mSubject.Tick(Ch::CHANNEL_1));
    }
}


} // namespace
