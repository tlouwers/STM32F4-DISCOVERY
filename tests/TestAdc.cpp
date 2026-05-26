/**
 * \file    TestAdc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Adc driver. Exercises the real driver
 *          (drivers/drivers/Adc) against the fake HAL ADC surface
 *          (tests/Fake/stm32f4xx_hal_adc.{h,cpp}).
 *
 * \details Coverage focuses on the driver's own logic: the Init config-id and
 *          per-option translation (channel / resolution / prescaler /
 *          sampling-time) plus the HAL_ADC_Init and HAL_ADC_ConfigChannel
 *          failure branches; the IsInit/Sleep lifecycle (incl. DeInit failure
 *          and the shared-ADC_IRQn NVIC-disable gate); the blocking GetValue
 *          start/poll/get/stop path with each HAL-failure point; the
 *          GetValueInterrupt start path; and the ADC_IRQn vector ->
 *          CallbackIRQ() -> HAL_ADC_IRQHandler -> HAL_ADC_ConvCpltCallback
 *          end-of-conversion dispatch (per instance) incl. the post-Sleep
 *          no-dispatch case. The register-level HAL behaviour itself is
 *          hardware-only and not unit-tested.
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

// Test subject -- the real Adc driver.
#include "drivers/Adc/Adc.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_adc.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs this C-linkage ISR entry point; the test invokes it
// directly to prove the shared ADC vector dispatches into the driver's
// CallbackIRQ() and on into the end-of-conversion callback.
extern "C" void ADC_IRQHandler(void);


namespace {

using Ch = Adc::Channel;


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from Adc::Config::Id(), used to
// prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

Adc::Config ValidConfig()
{
    return Adc::Config(0, Ch::CHANNEL_0);
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Adc_Test : public ::testing::Test
{
protected:
    Adc_Test()
        : mSubject(ADCInstance::ADC_1)
    {
        FakeADC_Reset();
    }

    Adc mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(Adc_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Adc_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(Adc_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Adc_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeADC_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Adc_Test, Init_HalConfigChannelFails_ReturnsFalseAndNotInit)
{
    FakeADC_SetConfigChannelResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

// Exercise every reachable switch arm in the private Get* translators by
// initialising with each enum value in turn.
TEST_F(Adc_Test, Init_EveryChannel_ReturnsTrue)
{
    for (uint8_t c = 0; c <= static_cast<uint8_t>(Ch::CHANNEL_15); ++c)
    {
        Adc adc(ADCInstance::ADC_1);
        EXPECT_TRUE(adc.Init(Adc::Config(0, static_cast<Ch>(c))));
    }
}

TEST_F(Adc_Test, Init_EveryResolution_ReturnsTrue)
{
    const Adc::Resolution resolutions[] = {
        Adc::Resolution::_6_BIT, Adc::Resolution::_8_BIT,
        Adc::Resolution::_10_BIT, Adc::Resolution::_12_BIT
    };
    for (const auto& res : resolutions)
    {
        Adc adc(ADCInstance::ADC_1);
        EXPECT_TRUE(adc.Init(Adc::Config(0, Ch::CHANNEL_0, res)));
    }
}

TEST_F(Adc_Test, Init_EveryPrescaler_ReturnsTrue)
{
    const Adc::Prescaler prescalers[] = {
        Adc::Prescaler::DIV2, Adc::Prescaler::DIV4,
        Adc::Prescaler::DIV6, Adc::Prescaler::DIV8
    };
    for (const auto& pre : prescalers)
    {
        Adc adc(ADCInstance::ADC_1);
        EXPECT_TRUE(adc.Init(Adc::Config(0, Ch::CHANNEL_0, Adc::Resolution::_12_BIT, pre)));
    }
}

TEST_F(Adc_Test, Init_EverySamplingTime_ReturnsTrue)
{
    const Adc::SamplingTime times[] = {
        Adc::SamplingTime::_3_CYCLES,   Adc::SamplingTime::_15_CYCLES,
        Adc::SamplingTime::_28_CYCLES,  Adc::SamplingTime::_56_CYCLES,
        Adc::SamplingTime::_84_CYCLES,  Adc::SamplingTime::_112_CYCLES,
        Adc::SamplingTime::_144_CYCLES, Adc::SamplingTime::_480_CYCLES
    };
    for (const auto& t : times)
    {
        Adc adc(ADCInstance::ADC_1);
        EXPECT_TRUE(adc.Init(Adc::Config(0, Ch::CHANNEL_0,
                                         Adc::Resolution::_12_BIT,
                                         Adc::Prescaler::DIV2, t)));
    }
}

// Each Adc instance routes to its own static callback slot + ADCx instance
// pointer; construct all three to exercise the SetInstance switch arms.
TEST_F(Adc_Test, Init_EachInstance_ReturnsTrue)
{
    Adc adc1(ADCInstance::ADC_1);
    Adc adc2(ADCInstance::ADC_2);
    Adc adc3(ADCInstance::ADC_3);

    EXPECT_TRUE(adc1.Init(ValidConfig()));
    EXPECT_TRUE(adc2.Init(ValidConfig()));
    EXPECT_TRUE(adc3.Init(ValidConfig()));
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(Adc_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Adc_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}

// With another instance still holding an active callback slot, Sleep must not
// take the all-slots-clear NVIC-disable branch; it still returns true.
TEST_F(Adc_Test, Sleep_OtherInstanceStillActive_ReturnsTrue)
{
    Adc adc2(ADCInstance::ADC_2);
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(adc2.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
}


/************************************************************************/
/* GetValue (blocking / polling)                                        */
/************************************************************************/
TEST_F(Adc_Test, GetValue_NotInit_ReturnsFalse)
{
    uint16_t value = 0;
    EXPECT_FALSE(mSubject.GetValue(value));
}

TEST_F(Adc_Test, GetValue_AfterInit_ReturnsTrueAndStoresValue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetConversionValue(0x0ABC);

    uint16_t value = 0;
    EXPECT_TRUE(mSubject.GetValue(value));
    EXPECT_EQ(0x0ABC, value);
}

TEST_F(Adc_Test, GetValue_HalStartFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetStartResult(HAL_ERROR);

    uint16_t value = 0;
    EXPECT_FALSE(mSubject.GetValue(value));
}

TEST_F(Adc_Test, GetValue_HalPollFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetPollResult(HAL_ERROR);

    uint16_t value = 0;
    EXPECT_FALSE(mSubject.GetValue(value));
}

TEST_F(Adc_Test, GetValue_HalStopFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetStopResult(HAL_ERROR);

    uint16_t value = 0;
    EXPECT_FALSE(mSubject.GetValue(value));
}


/************************************************************************/
/* GetValueInterrupt                                                    */
/************************************************************************/
TEST_F(Adc_Test, GetValueInterrupt_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.GetValueInterrupt([](uint16_t) {}));
}

TEST_F(Adc_Test, GetValueInterrupt_AfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.GetValueInterrupt([](uint16_t) {}));
}

TEST_F(Adc_Test, GetValueInterrupt_HalStartItFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeADC_SetStartItResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.GetValueInterrupt([](uint16_t) {}));
}


/************************************************************************/
/* IRQ dispatch / end-of-conversion                                     */
/************************************************************************/
TEST_F(Adc_Test, IRQHandler_AfterGetValueInterrupt_DispatchesEndOfConversion)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    uint16_t captured  = 0;
    bool     wasCalled = false;
    ASSERT_TRUE(mSubject.GetValueInterrupt([&](uint16_t v) { captured = v; wasCalled = true; }));

    FakeADC_SetConversionValue(0x07D0);
    ASSERT_EQ(0, FakeADC_IRQHandlerCallCount());

    // Firing the shared ADC vector must route through the registered
    // callbackIRQ into HAL_ADC_IRQHandler, which models the EOC and dispatches
    // the end-of-conversion callback with the converted value.
    ADC_IRQHandler();

    EXPECT_EQ(1, FakeADC_IRQHandlerCallCount());
    EXPECT_TRUE(wasCalled);
    EXPECT_EQ(0x07D0, captured);
}

TEST_F(Adc_Test, IRQHandler_Adc3Instance_DispatchesEndOfConversion)
{
    Adc adc3(ADCInstance::ADC_3);
    ASSERT_TRUE(adc3.Init(ValidConfig()));

    bool wasCalled = false;
    ASSERT_TRUE(adc3.GetValueInterrupt([&](uint16_t) { wasCalled = true; }));

    ADC_IRQHandler();

    EXPECT_TRUE(wasCalled);
}

TEST_F(Adc_Test, IRQHandler_AfterSleep_DoesNotDispatch)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    bool wasCalled = false;
    ASSERT_TRUE(mSubject.GetValueInterrupt([&](uint16_t) { wasCalled = true; }));
    ASSERT_TRUE(mSubject.Sleep());   // DisconnectCallbacks() drops both slots.

    ADC_IRQHandler();

    EXPECT_EQ(0, FakeADC_IRQHandlerCallCount());
    EXPECT_FALSE(wasCalled);
}


} // namespace
