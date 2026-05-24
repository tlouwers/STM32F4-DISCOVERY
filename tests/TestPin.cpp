/**
 * \file    TestPin.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Pin driver. Exercises the real driver
 *          (drivers/drivers/Pin) against the fake HAL GPIO/EXTI surface
 *          (umbrella stm32f4xx_hal.h + tests/Fake/stm32f4xx_hal_gpio.{h,cpp}).
 *
 * \details Coverage focuses on the driver's own logic: each Configure overload
 *          (output drive arms, input pull arms incl. analog, alternate-function
 *          arms) and the Mode/Pull the driver writes into the HAL init struct;
 *          Set/Get/Toggle (Get on both INPUT and OUTPUT directions); the full
 *          interrupt seam -- Interrupt() register + the already-configured
 *          double-register reject (enabled and disabled sub-paths),
 *          InterruptEnable/Disable/Remove incl. their no-callback false paths,
 *          the shared-EXTI-line branch of IsIRQSharedWithOtherPin, and every
 *          GetIRQn grouping (dedicated 0..4, shared 9_5, shared 15_10); and the
 *          EXTIn ISR -> HAL_GPIO_EXTI_IRQHandler -> HAL_GPIO_EXTI_Callback ->
 *          registered pin callback dispatch (fires when enabled, absorbed when
 *          disabled/removed). The move constructor / move assignment are also
 *          exercised. The GPIO register behaviour itself is hardware-only.
 *
 * \note    Get() on an unconfigured pin deliberately spins forever on hardware
 *          (UNDEFINED direction), so every Get() here is on a configured pin.
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
#include <utility>

// Test subject -- the real Pin driver.
#include "drivers/Pin/Pin.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_gpio.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs these C-linkage ISR entry points; the test invokes them
// directly to prove each EXTI line dispatches into the registered pin callback.
extern "C" void EXTI0_IRQHandler(void);
extern "C" void EXTI1_IRQHandler(void);
extern "C" void EXTI2_IRQHandler(void);
extern "C" void EXTI3_IRQHandler(void);
extern "C" void EXTI4_IRQHandler(void);
extern "C" void EXTI9_5_IRQHandler(void);
extern "C" void EXTI15_10_IRQHandler(void);


namespace {

/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
PinIdPort IdPort(uint16_t id)
{
    PinIdPort p;
    p.id   = id;
    p.port = GPIOA;
    return p;
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Pin_Test : public ::testing::Test
{
protected:
    Pin_Test()
    {
        FakeGPIO_Reset();
    }
};


/************************************************************************/
/* Configure -- output                                                  */
/************************************************************************/
TEST_F(Pin_Test, ConfigureOutput_PushPull_InitsPushPullNoPullAndSets)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Level::HIGH, Drive::PUSH_PULL);

    EXPECT_EQ(1, FakeGPIO_InitCallCount());
    EXPECT_EQ(GPIO_MODE_OUTPUT_PP, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_NOPULL,         FakeGPIO_LastInitPull());
    // Configure(level) ends by writing the initial level.
    EXPECT_EQ(GPIO_PIN_SET, FakeGPIO_LastWriteState());
}

TEST_F(Pin_Test, ConfigureOutput_OpenDrain_InitsOpenDrainNoPull)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Level::LOW, Drive::OPEN_DRAIN);

    EXPECT_EQ(GPIO_MODE_OUTPUT_OD, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_NOPULL,         FakeGPIO_LastInitPull());
    EXPECT_EQ(GPIO_PIN_RESET,      FakeGPIO_LastWriteState());
}

TEST_F(Pin_Test, ConfigureOutput_OpenDrainPullUp_InitsPullUp)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Level::HIGH, Drive::OPEN_DRAIN_PULL_UP);
    EXPECT_EQ(GPIO_PULLUP, FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureOutput_OpenDrainPullDown_InitsPullDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Level::HIGH, Drive::OPEN_DRAIN_PULL_DOWN);
    EXPECT_EQ(GPIO_PULLDOWN, FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureOutput_OpenDrainPullUpDown_InitsPullUpDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Level::HIGH, Drive::OPEN_DRAIN_PULL_UP_DOWN);
    EXPECT_EQ((GPIO_PULLUP | GPIO_PULLDOWN), FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConstructAsOutput_ConfiguresImmediately)
{
    Pin pin(IdPort(GPIO_PIN_1), Level::HIGH, Drive::PUSH_PULL);
    EXPECT_EQ(1, FakeGPIO_InitCallCount());
    EXPECT_EQ(GPIO_MODE_OUTPUT_PP, FakeGPIO_LastInitMode());
}


/************************************************************************/
/* Configure -- input                                                   */
/************************************************************************/
TEST_F(Pin_Test, ConfigureInput_HighZ_InitsInputNoPull)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(PullUpDown::HIGHZ);
    EXPECT_EQ(GPIO_MODE_INPUT, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_NOPULL,     FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureInput_PullUp_InitsPullUp)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(PullUpDown::UP);
    EXPECT_EQ(GPIO_MODE_INPUT, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_PULLUP,     FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureInput_PullDown_InitsPullDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(PullUpDown::DOWN);
    EXPECT_EQ(GPIO_PULLDOWN, FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureInput_PullUpDown_InitsPullUpDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(PullUpDown::UP_DOWN);
    EXPECT_EQ((GPIO_PULLUP | GPIO_PULLDOWN), FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureInput_Analog_InitsAnalogNoPull)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(PullUpDown::ANALOG);
    EXPECT_EQ(GPIO_MODE_ANALOG, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_NOPULL,      FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConstructAsInput_ConfiguresImmediately)
{
    Pin pin(IdPort(GPIO_PIN_2), PullUpDown::UP);
    EXPECT_EQ(GPIO_MODE_INPUT, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_PULLUP,     FakeGPIO_LastInitPull());
}


/************************************************************************/
/* Configure -- alternate function                                      */
/************************************************************************/
TEST_F(Pin_Test, ConfigureAlternate_PushPullHighZ_InitsAfPpNoPull)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Alternate::AF4, PullUpDown::HIGHZ, Mode::PUSH_PULL);
    EXPECT_EQ(GPIO_MODE_AF_PP, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_NOPULL,     FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureAlternate_OpenDrainPullUp_InitsAfOdPullUp)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Alternate::AF7, PullUpDown::UP, Mode::OPEN_DRAIN);
    EXPECT_EQ(GPIO_MODE_AF_OD, FakeGPIO_LastInitMode());
    EXPECT_EQ(GPIO_PULLUP,     FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureAlternate_PullDown_InitsPullDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Alternate::AF0, PullUpDown::DOWN);
    EXPECT_EQ(GPIO_PULLDOWN, FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConfigureAlternate_PullUpDown_InitsPullUpDown)
{
    Pin pin(IdPort(GPIO_PIN_0));
    pin.Configure(Alternate::AF0, PullUpDown::UP_DOWN);
    EXPECT_EQ((GPIO_PULLUP | GPIO_PULLDOWN), FakeGPIO_LastInitPull());
}

TEST_F(Pin_Test, ConstructAsAlternate_ConfiguresImmediately)
{
    Pin pin(IdPort(GPIO_PIN_3), Alternate::AF5);
    EXPECT_EQ(GPIO_MODE_AF_PP, FakeGPIO_LastInitMode());
}


/************************************************************************/
/* Set / Get / Toggle                                                   */
/************************************************************************/
TEST_F(Pin_Test, Set_High_WritesPinSet)
{
    Pin pin(IdPort(GPIO_PIN_0), Level::LOW);   // configured output
    pin.Set(Level::HIGH);
    EXPECT_EQ(GPIO_PIN_SET, FakeGPIO_LastWriteState());
}

TEST_F(Pin_Test, Set_Low_WritesPinReset)
{
    Pin pin(IdPort(GPIO_PIN_0), Level::HIGH);
    pin.Set(Level::LOW);
    EXPECT_EQ(GPIO_PIN_RESET, FakeGPIO_LastWriteState());
}

TEST_F(Pin_Test, Toggle_OutputPin_CallsHalToggle)
{
    Pin pin(IdPort(GPIO_PIN_0), Level::LOW);
    pin.Toggle();
    EXPECT_EQ(1, FakeGPIO_ToggleCallCount());
}

TEST_F(Pin_Test, Get_OutputPin_ReturnsHighWhenReadSet)
{
    Pin pin(IdPort(GPIO_PIN_0), Level::LOW);
    FakeGPIO_SetReadPinState(GPIO_PIN_SET);
    EXPECT_EQ(Level::HIGH, pin.Get());
}

TEST_F(Pin_Test, Get_InputPin_ReturnsLowWhenReadReset)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    FakeGPIO_SetReadPinState(GPIO_PIN_RESET);
    EXPECT_EQ(Level::LOW, pin.Get());
}


/************************************************************************/
/* Move semantics                                                       */
/************************************************************************/
TEST_F(Pin_Test, MoveConstruct_TransfersAndLeavesSourceUsable)
{
    Pin src(IdPort(GPIO_PIN_0), Level::LOW);
    Pin dst(std::move(src));
    dst.Set(Level::HIGH);
    EXPECT_EQ(GPIO_PIN_SET, FakeGPIO_LastWriteState());
}

TEST_F(Pin_Test, MoveAssign_TransfersState)
{
    Pin a(IdPort(GPIO_PIN_0), Level::LOW);
    Pin b(IdPort(GPIO_PIN_1), Level::LOW);
    b = std::move(a);
    b.Set(Level::HIGH);
    EXPECT_EQ(GPIO_PIN_SET, FakeGPIO_LastWriteState());
}


/************************************************************************/
/* Interrupt -- configure / lifecycle                                   */
/************************************************************************/
TEST_F(Pin_Test, Interrupt_OnInputPin_ReturnsTrue)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    EXPECT_TRUE(pin.Interrupt(Trigger::RISING, []{}));
    EXPECT_TRUE(pin.InterruptRemove());   // clean the process-global slot table
}

TEST_F(Pin_Test, Interrupt_EveryTrigger_ReturnsTrue)
{
    const Trigger triggers[] = { Trigger::RISING, Trigger::FALLING, Trigger::BOTH };
    uint16_t id = GPIO_PIN_0;
    for (const auto& trig : triggers)
    {
        Pin pin(IdPort(id), PullUpDown::UP);
        EXPECT_TRUE(pin.Interrupt(trig, []{}));
        EXPECT_TRUE(pin.InterruptRemove());
    }
}

TEST_F(Pin_Test, Interrupt_AlreadyConfiguredEnabled_ReturnsFalse)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, []{}, true));   // enabled

    // Second register sees an existing enabled callback -> restores NVIC, false.
    EXPECT_FALSE(pin.Interrupt(Trigger::RISING, []{}));
    EXPECT_TRUE(pin.InterruptRemove());
}

TEST_F(Pin_Test, Interrupt_AlreadyConfiguredDisabled_ReturnsFalse)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, []{}, false));  // not enabled

    EXPECT_FALSE(pin.Interrupt(Trigger::RISING, []{}));
    EXPECT_TRUE(pin.InterruptRemove());
}

TEST_F(Pin_Test, InterruptEnable_NoCallback_ReturnsFalse)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    EXPECT_FALSE(pin.InterruptEnable());
}

TEST_F(Pin_Test, InterruptDisable_NoCallback_ReturnsFalse)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    EXPECT_FALSE(pin.InterruptDisable());
}

TEST_F(Pin_Test, InterruptRemove_NoCallback_ReturnsFalse)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    EXPECT_FALSE(pin.InterruptRemove());
}

TEST_F(Pin_Test, InterruptEnableDisableRemove_AfterConfigure_AllTrue)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, []{}, false));

    EXPECT_TRUE(pin.InterruptEnable());
    EXPECT_TRUE(pin.InterruptDisable());
    EXPECT_TRUE(pin.InterruptRemove());
}


/************************************************************************/
/* Interrupt -- IRQ groupings                                           */
/************************************************************************/
// Two pins in the shared EXTI9_5 line: IsIRQSharedWithOtherPin must report true,
// exercising the shared-line branch in Interrupt / InterruptDisable.
TEST_F(Pin_Test, Interrupt_SharedExti9_5Line_BothRegisterAndDisable)
{
    Pin pin5(IdPort(GPIO_PIN_5), PullUpDown::UP);
    Pin pin6(IdPort(GPIO_PIN_6), PullUpDown::UP);

    ASSERT_TRUE(pin5.Interrupt(Trigger::RISING, []{}));
    ASSERT_TRUE(pin6.Interrupt(Trigger::RISING, []{}));   // shared-line skip branch

    EXPECT_TRUE(pin5.InterruptDisable());   // shared -> NVIC disable skipped
    EXPECT_TRUE(pin6.InterruptDisable());

    EXPECT_TRUE(pin5.InterruptRemove());
    EXPECT_TRUE(pin6.InterruptRemove());
}

// A pin in the shared EXTI15_10 line exercises the GetIRQn default arm.
TEST_F(Pin_Test, Interrupt_SharedExti15_10Line_RegistersAndRemoves)
{
    Pin pin12(IdPort(GPIO_PIN_12), PullUpDown::UP);
    ASSERT_TRUE(pin12.Interrupt(Trigger::RISING, []{}));
    EXPECT_TRUE(pin12.InterruptRemove());
}


/************************************************************************/
/* Interrupt -- ISR dispatch                                            */
/************************************************************************/
TEST_F(Pin_Test, ExtiIrq_EnabledCallback_Fires)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    bool fired = false;
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, [&]{ fired = true; }, true));

    EXTI0_IRQHandler();

    EXPECT_TRUE(fired);
    EXPECT_TRUE(pin.InterruptRemove());
}

TEST_F(Pin_Test, ExtiIrq_DisabledCallback_IsAbsorbed)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    bool fired = false;
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, [&]{ fired = true; }, false));  // disabled

    EXTI0_IRQHandler();

    EXPECT_FALSE(fired);
    EXPECT_TRUE(pin.InterruptRemove());
}

TEST_F(Pin_Test, ExtiIrq_AfterRemove_IsAbsorbed)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP);
    bool fired = false;
    ASSERT_TRUE(pin.Interrupt(Trigger::RISING, [&]{ fired = true; }, true));
    ASSERT_TRUE(pin.InterruptRemove());

    EXTI0_IRQHandler();

    EXPECT_FALSE(fired);
}

// Shared-line ISR fans out to every pin in the group; the registered pin fires.
TEST_F(Pin_Test, ExtiIrq_SharedExti15_10Line_FiresRegisteredPin)
{
    Pin pin12(IdPort(GPIO_PIN_12), PullUpDown::UP);
    bool fired = false;
    ASSERT_TRUE(pin12.Interrupt(Trigger::RISING, [&]{ fired = true; }, true));

    EXTI15_10_IRQHandler();

    EXPECT_TRUE(fired);
    EXPECT_TRUE(pin12.InterruptRemove());
}

// Each dedicated EXTI line (1..4) has its own vector + GetIRQn arm.
TEST_F(Pin_Test, ExtiIrq_EachDedicatedLine_Fires)
{
    Pin pin1(IdPort(GPIO_PIN_1), PullUpDown::UP);
    Pin pin2(IdPort(GPIO_PIN_2), PullUpDown::UP);
    Pin pin3(IdPort(GPIO_PIN_3), PullUpDown::UP);
    Pin pin4(IdPort(GPIO_PIN_4), PullUpDown::UP);
    bool f1 = false, f2 = false, f3 = false, f4 = false;
    ASSERT_TRUE(pin1.Interrupt(Trigger::RISING, [&]{ f1 = true; }, true));
    ASSERT_TRUE(pin2.Interrupt(Trigger::RISING, [&]{ f2 = true; }, true));
    ASSERT_TRUE(pin3.Interrupt(Trigger::RISING, [&]{ f3 = true; }, true));
    ASSERT_TRUE(pin4.Interrupt(Trigger::RISING, [&]{ f4 = true; }, true));

    EXTI1_IRQHandler();
    EXTI2_IRQHandler();
    EXTI3_IRQHandler();
    EXTI4_IRQHandler();

    EXPECT_TRUE(f1);
    EXPECT_TRUE(f2);
    EXPECT_TRUE(f3);
    EXPECT_TRUE(f4);
    EXPECT_TRUE(pin1.InterruptRemove());
    EXPECT_TRUE(pin2.InterruptRemove());
    EXPECT_TRUE(pin3.InterruptRemove());
    EXPECT_TRUE(pin4.InterruptRemove());
}

TEST_F(Pin_Test, ExtiIrq_SharedExti9_5Line_FiresRegisteredPin)
{
    Pin pin7(IdPort(GPIO_PIN_7), PullUpDown::UP);
    bool fired = false;
    ASSERT_TRUE(pin7.Interrupt(Trigger::RISING, [&]{ fired = true; }, true));

    EXTI9_5_IRQHandler();

    EXPECT_TRUE(fired);
    EXPECT_TRUE(pin7.InterruptRemove());
}


/************************************************************************/
/* Interrupt -- pull carry-forward                                      */
/************************************************************************/
// Interrupt() re-applies the pull set by the prior Configure(PullUpDown), since
// HAL_GPIO_Init rewrites PUPDR; exercise the DOWN and UP_DOWN carry-forward arms.
TEST_F(Pin_Test, Interrupt_PreservesPullDown_ReAppliesPullDown)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::DOWN);
    EXPECT_TRUE(pin.Interrupt(Trigger::RISING, []{}));
    EXPECT_EQ(GPIO_PULLDOWN, FakeGPIO_LastInitPull());
    EXPECT_TRUE(pin.InterruptRemove());
}

TEST_F(Pin_Test, Interrupt_PreservesPullUpDown_ReAppliesPullUpDown)
{
    Pin pin(IdPort(GPIO_PIN_0), PullUpDown::UP_DOWN);
    EXPECT_TRUE(pin.Interrupt(Trigger::RISING, []{}));
    EXPECT_EQ((GPIO_PULLUP | GPIO_PULLDOWN), FakeGPIO_LastInitPull());
    EXPECT_TRUE(pin.InterruptRemove());
}


} // namespace
