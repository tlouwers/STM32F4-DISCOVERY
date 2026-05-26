/**
 * \file    Application.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for StandupCounter.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/StandupCounter/target/Src
 *
 * \details StandupCounter with 8x8 LED display and buzzer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <functional>
#include "Application.hpp"
#include "board/BoardConfig.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, configures pins and callbacks.
 */
Application::Application() :
    mButton(PIN_BUTTON, PullUpDown::HIGHZ),             // Externally pulled down
    mLedGreen(PIN_LED_GREEN, Level::LOW),               // Off
    mLedOrange(PIN_LED_ORANGE, Level::LOW),
    mLedRed(PIN_LED_RED, Level::LOW),
    mLedBlue(PIN_LED_BLUE, Level::LOW),
    mChipSelect(PIN_SPI2_CS, Alternate::AF5),
    mPWMOut(PIN_PWM_CH1, Alternate::AF1),
    mPWM(PwmTimerInstance::TIMER_2),
    mSPI(SPIInstance::SPI_2),
    mMatrix(mSPI, PIN_SPI2_CS),
    mLogic(mMatrix, mPWM, mLedGreen, mLedOrange, mLedRed, mDelay)
{
    // Note: button conflicts with the accelerometer int1 pin. This is a board layout issue.
    mButton.Interrupt(Trigger::RISING, [this]() { mLogic.OnButtonPressed(); } );
}

/**
 * \brief   Initialize the various peripherals, configures components and show
 *          the user the application is starting using the leds.
 * \returns True if init is successful, else false.
 */
bool Application::Init()
{
    mLedGreen.Set(Level::HIGH);
    HAL_Delay(750);

    // Actual Init()
    bool result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    EXPECT(result);

    result &= mMatrix.Init(HI_M1388AR::Config(8));
    EXPECT(result);


    result &= mPWM.Init(PWM::Config(500));
    EXPECT(result);

    result &= mPWM.ConfigureChannel(PWM::ChannelConfig(PWM::Channel::Channel_1, 0.5f, PWM::Polarity::High));
    EXPECT(result);


    mLedGreen.Set(Level::LOW);

    return result;
}

/**
 * \brief   Main process loop of the application. This method is to be called
 *          often and acts as the main processor of data of the application.
 */
void Application::Process()
{
    mLogic.Process();
}

/**
 * \brief   Error handler, acts as visual indicator to the user that the
 *          application entered an error state by toggling the red led.
 */
void Application::Error()
{
#ifdef DEBUG
    __asm volatile("BKPT #01");
#endif

    mLedGreen.Set(Level::LOW);
    mLedOrange.Set(Level::LOW);
    mLedRed.Set(Level::LOW);
    mLedBlue.Set(Level::LOW);

    while (1)
    {
        mLedRed.Toggle();
        HAL_Delay(250);
    }
}
