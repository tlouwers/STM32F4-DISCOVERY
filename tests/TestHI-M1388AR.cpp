/**
 * \file    TestHI-M1388AR.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the HI-M1388AR LED-matrix component, driving
 *          a Mock_SPI / Mock_Pin pair through the real driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

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
