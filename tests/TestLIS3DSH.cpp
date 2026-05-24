/**
 * \file    TestLIS3DSH.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the LIS3DSH accelerometer component, driving
 *          a Mock_SPI / Mock_Pin pair through the real driver and exercising
 *          the caller-supplied read-buffer validation introduced in L5c.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "gtest/gtest.h"


// Test subject
#include "components/LIS3DSH/LIS3DSH.hpp"

// Supporting files
#include "board/BoardConfig.hpp"

// Mock
#include "Mock/Mock_SPI.hpp"


using ::testing::DoAll;
using ::testing::InvokeArgument;


// The real Pin driver (linked binary-wide) installs this C-linkage EXTI entry
// point; PIN_MOTION_INT1 is GPIO_PIN_0, so firing EXTI0 drives the INT1 seam.
extern "C" void EXTI0_IRQHandler(void);


namespace {


// Test fixture for LIS3DSH - accelerometer.
class LIS3DSH_Test : public ::testing::Test
{
protected:
    Mock_SPI spi;

    LIS3DSH_Test() :
        mSubject(spi, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2)
    {
        // Initialize test matter
    }

    LIS3DSH mSubject;
};


TEST_F(LIS3DSH_Test, Init_IsInit_Sleep)
{
    EXPECT_FALSE(mSubject.IsInit());

    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                              LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Sleep());

    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(LIS3DSH_Test, Enable)
{
    EXPECT_FALSE(mSubject.Enable());   // Not initialized yet

    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                              LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.Enable());
}

TEST_F(LIS3DSH_Test, Disable)
{
    EXPECT_FALSE(mSubject.Disable());   // Not initialized yet

    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                              LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.Disable());
}

TEST_F(LIS3DSH_Test, RetrieveAxesData)
{
    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                              LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    uint8_t motionArray[25 * 3 * 2] = {};       // 25 samples, X,Y,Z, 2 bytes/sample -- FIFO size
    uint8_t motionLength = sizeof(motionArray); // Normally returned from interupt when data received

    EXPECT_TRUE(mSubject.RetrieveAxesData(motionArray, motionLength));

    // Cannot check contents, this is filled in via SPI ReadDMA when Pin interrupt occurs.
}

TEST_F(LIS3DSH_Test, Init_NullBuffer_ReturnsFalseAndStaysUninitialised)
{
    EXPECT_FALSE(mSubject.Init(LIS3DSH::Config(nullptr, LIS3DSH::FIFO_READ_BUFFER_SIZE, true,
                                               LIS3DSH::SampleFrequency::_50_Hz)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(LIS3DSH_Test, Init_UndersizedFifoBuffer_ReturnsFalseAndStaysUninitialised)
{
    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE - 1] = {};
    EXPECT_FALSE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                               LIS3DSH::SampleFrequency::_50_Hz)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(LIS3DSH_Test, Init_UndersizedSingleBuffer_ReturnsFalseAndStaysUninitialised)
{
    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE - 1] = {};
    EXPECT_FALSE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                               LIS3DSH::SampleFrequency::_50_Hz)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(LIS3DSH_Test, Init_HardwareFifo_ReturnsTrue)
{
    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                              LIS3DSH::SampleFrequency::_100_Hz)));
    EXPECT_TRUE(mSubject.IsInit());
}

// A non-empty FIFO at Init must be drained (read out) before use; this drives
// ClearFifo's non-empty branch incl. the second-pass single-sample drain.
TEST_F(LIS3DSH_Test, Init_HardwareFifo_NonEmptyFifo_DrainsThenInitialises)
{
    spi.SetFifoSrc(0x05);   // 5 queued samples, FIFO_EMPTY bit clear

    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE] = {};
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                              LIS3DSH::SampleFrequency::_100_Hz)));
    EXPECT_TRUE(mSubject.IsInit());
}

// When the sensor never reports its identifier, SelfTest exhausts its retries
// (the HAL_Delay back-off arm) and Init fails.
TEST_F(LIS3DSH_Test, Init_WrongWhoAmI_FailsSelfTestAndStaysUninitialised)
{
    spi.SetWhoAmI(0x00);    // never matches the LIS3DSH identifier (0x3F)

    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    EXPECT_FALSE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                               LIS3DSH::SampleFrequency::_50_Hz)));
    EXPECT_FALSE(mSubject.IsInit());
}

// Hardware-FIFO Enable/Disable take the FIFO_CTRL read-modify-write branch
// (and Enable applies the Stream FMODE), unlike the data-ready (no-FIFO) path.
TEST_F(LIS3DSH_Test, EnableDisable_HardwareFifo_TogglesAcquisition)
{
    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE] = {};
    ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                              LIS3DSH::SampleFrequency::_100_Hz)));

    EXPECT_TRUE(mSubject.Enable());
    EXPECT_TRUE(mSubject.Disable());
}

// Every Config enum value must translate -- exercises GetSampleFrequencyAsODR,
// GetScaleAsFSCALE and GetAntiAliasingFilterAsBW across all their arms.
TEST_F(LIS3DSH_Test, Init_EveryConfigEnum_ReturnsTrue)
{
    const LIS3DSH::SampleFrequency freqs[] = {
        LIS3DSH::SampleFrequency::_3_125_Hz, LIS3DSH::SampleFrequency::_6_25_Hz,
        LIS3DSH::SampleFrequency::_12_5_Hz,  LIS3DSH::SampleFrequency::_25_Hz,
        LIS3DSH::SampleFrequency::_50_Hz,    LIS3DSH::SampleFrequency::_100_Hz,
        LIS3DSH::SampleFrequency::_400_Hz,   LIS3DSH::SampleFrequency::_800_Hz,
        LIS3DSH::SampleFrequency::_1600_Hz,
    };
    const LIS3DSH::Scale scales[] = {
        LIS3DSH::Scale::_2_G, LIS3DSH::Scale::_4_G, LIS3DSH::Scale::_6_G,
        LIS3DSH::Scale::_8_G, LIS3DSH::Scale::_16_G,
    };
    const LIS3DSH::AntiAliasingFilter filters[] = {
        LIS3DSH::AntiAliasingFilter::_50_Hz,  LIS3DSH::AntiAliasingFilter::_200_Hz,
        LIS3DSH::AntiAliasingFilter::_400_Hz, LIS3DSH::AntiAliasingFilter::_800_Hz,
    };

    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};

    for (const LIS3DSH::SampleFrequency freq : freqs)
    {
        ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false, freq)));
        ASSERT_TRUE(mSubject.Sleep());
    }
    for (const LIS3DSH::Scale scale : scales)
    {
        ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                                  LIS3DSH::SampleFrequency::_50_Hz, scale)));
        ASSERT_TRUE(mSubject.Sleep());
    }
    for (const LIS3DSH::AntiAliasingFilter filter : filters)
    {
        ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                                  LIS3DSH::SampleFrequency::_50_Hz,
                                                  LIS3DSH::Scale::_2_G, filter)));
        ASSERT_TRUE(mSubject.Sleep());
    }
}

// INT1 (data-ready) -> CallbackInt1 starts a DMA read whose completion runs
// ReadAxesCompleted, which must notify the registered handler with one sample.
TEST_F(LIS3DSH_Test, Int1Interrupt_DataReady_NotifiesHandlerWithSampleLength)
{
    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    bool    called   = false;
    uint8_t reported = 0;
    mSubject.SetHandler([&](uint8_t length) { called = true; reported = length; });

    ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                              LIS3DSH::SampleFrequency::_50_Hz)));
    ASSERT_TRUE(mSubject.Enable());

    // Drive the DMA-read completion synchronously so ReadAxesCompleted runs.
    EXPECT_CALL(spi, ReadDMA(_, _, _))
        .WillOnce(DoAll(InvokeArgument<2>(), Return(true)));

    EXTI0_IRQHandler();

    EXPECT_TRUE(called);
    EXPECT_EQ(6u, reported);   // SINGLE_READ_BUFFER_SIZE
}

// Same seam with the hardware FIFO: the handler is told the full FIFO size.
TEST_F(LIS3DSH_Test, Int1Interrupt_HardwareFifo_NotifiesHandlerWithFifoSize)
{
    uint8_t buf[LIS3DSH::FIFO_READ_BUFFER_SIZE] = {};
    bool    called   = false;
    uint8_t reported = 0;
    mSubject.SetHandler([&](uint8_t length) { called = true; reported = length; });

    ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), true,
                                              LIS3DSH::SampleFrequency::_100_Hz)));
    ASSERT_TRUE(mSubject.Enable());

    EXPECT_CALL(spi, ReadDMA(_, _, _))
        .WillOnce(DoAll(InvokeArgument<2>(), Return(true)));

    EXTI0_IRQHandler();

    EXPECT_TRUE(called);
    EXPECT_EQ(static_cast<uint8_t>(LIS3DSH::FIFO_READ_BUFFER_SIZE), reported);
}

// When the address write fails, CallbackInt1 must NOT start the DMA read and
// must raise ChipSelect itself (the bus-not-left-low guard).
TEST_F(LIS3DSH_Test, Int1Interrupt_AddressWriteFails_DoesNotStartDmaRead)
{
    uint8_t buf[LIS3DSH::SINGLE_READ_BUFFER_SIZE] = {};
    bool    called = false;
    mSubject.SetHandler([&](uint8_t) { called = true; });

    ASSERT_TRUE(mSubject.Init(LIS3DSH::Config(buf, sizeof(buf), false,
                                              LIS3DSH::SampleFrequency::_50_Hz)));
    ASSERT_TRUE(mSubject.Enable());

    // Fail the single address-byte WriteBlocking inside CallbackInt1.
    EXPECT_CALL(spi, WriteBlocking(_, _))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(spi, ReadDMA(_, _, _)).Times(0);

    EXTI0_IRQHandler();

    EXPECT_FALSE(called);
}


} // namespace
