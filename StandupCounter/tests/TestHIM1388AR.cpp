#include "gtest/gtest.h"


// Test subject
#include "components/HIM1388AR/HIM1388AR.hpp"

// Supporting files
#include "board/BoardConfig.hpp"
#include "components/HIM1388AR/HIM1388ARLib.hpp"

// Mock
#include "Mock/Mock_SPI.hpp"


namespace {


// Test fixture for HI-M1388AR - 8x8 LED matrix display.
class HIM1388AR_Test : public ::testing::Test
{
protected:
    Mock_SPI spi;

    HIM1388AR_Test() :
        mSubject(spi, PIN_SPI2_CS)
    {
        // Initialize test matter
    }

    HIM1388AR mSubject;
};


TEST_F(HIM1388AR_Test, Init_IsInit_Sleep)
{
    EXPECT_FALSE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Init(HIM1388AR::Config(8)));

    EXPECT_TRUE(mSubject.IsInit());

    EXPECT_TRUE(mSubject.Sleep());

    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(HIM1388AR_Test, ClearDisplay)
{
    EXPECT_FALSE(mSubject.ClearDisplay());   // Not initialized yet

    EXPECT_TRUE(mSubject.Init(HIM1388AR::Config(8)));

    EXPECT_TRUE(mSubject.ClearDisplay());
}

TEST_F(HIM1388AR_Test, WriteDigits)
{
    EXPECT_FALSE(mSubject.WriteDigits(symbol_smiley));  // Not initialized yet

    EXPECT_TRUE(mSubject.Init(HIM1388AR::Config(8)));

    EXPECT_TRUE(mSubject.WriteDigits(symbol_smiley));
}


} // namespace
