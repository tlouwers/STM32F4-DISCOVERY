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


} // namespace
