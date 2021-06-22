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

    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Sleep());

    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(LIS3DSH_Test, Enable)
{
    EXPECT_FALSE(mSubject.Enable());   // Not initialized yet

    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.Enable());
}

TEST_F(LIS3DSH_Test, Disable)
{
    EXPECT_FALSE(mSubject.Disable());   // Not initialized yet

    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    EXPECT_TRUE(mSubject.Disable());
}

TEST_F(LIS3DSH_Test, RetrieveAxesData)
{
    EXPECT_TRUE(mSubject.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz,
                                              LIS3DSH::Scale::_2_G,
                                              LIS3DSH::AntiAliasingFilter::_200_Hz)));

    uint8_t motionArray[25 * 3 * 2] = {};       // 25 samples, X,Y,Z, 2 bytes/sample -- FIFO size
    uint8_t motionLength = sizeof(motionArray); // Normally returned from interupt when data received

    EXPECT_TRUE(mSubject.RetrieveAxesData(motionArray, motionLength));

    // Cannot check contents, this is filled in via SPI ReadDMA when Pin interrupt occurs.
}


} // namespace
