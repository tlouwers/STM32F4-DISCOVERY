#include "gtest/gtest.h"


// Test subject
#include "components/HI-M1388AR/HI-M1388AR.hpp"

// Supporting files
#include "board/BoardConfig.hpp"
#include "components/HI-M1388AR/HI-M1388AR_Lib.hpp"

// Mock
#include "Mock/Mock_SPI.hpp"


namespace {


// Test fixture for HI-M1388AR - 8x8 LED matrix display.
class HI_M1388AR_Test : public ::testing::Test
{
protected:
    Mock_SPI spi;

    HI_M1388AR_Test() :
        mSubject(spi, PIN_SPI2_CS)
    {
        // Initialize test matter
    }

    HI_M1388AR mSubject;
};


TEST_F(HI_M1388AR_Test, Init_IsInit_Sleep)
{
    EXPECT_FALSE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Init(HI_M1388AR::Config(8)));

    EXPECT_TRUE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Sleep());

    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(HI_M1388AR_Test, ClearDisplay)
{
    EXPECT_FALSE(mSubject.ClearDisplay());   // Not initialized yet

    EXPECT_TRUE(mSubject.Init(HI_M1388AR::Config(8)));

    EXPECT_TRUE(mSubject.ClearDisplay());
}

TEST_F(HI_M1388AR_Test, WriteDigits)
{
    EXPECT_FALSE(mSubject.WriteDigits(symbol_smiley));  // Not initialized yet

    EXPECT_TRUE(mSubject.Init(HI_M1388AR::Config(8)));

    EXPECT_TRUE(mSubject.WriteDigits(symbol_smiley));
}


} // namespace
