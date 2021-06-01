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
#include "components/HIM1388AR/HIM1388ARLib.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint32_t MAX_LOOP_COUNT = 10;      // Do not set above 10, display logic can represent only a single digit!
static constexpr uint32_t LONG_DELAY_MS  = 105000;


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
    mButtonPressed(false)
{
    // Note: button conflicts with the accelerometer int1 pin. This is a board layout issue.
    mButton.Interrupt(Trigger::RISING, [this]() { this->ButtonPressedCallback(); } );
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

    result &= mMatrix.Init(HIM1388AR::Config(8));
    EXPECT(result);


    result &= mPWM.Init(PWM::Config(500));
    EXPECT(result);

    result &= mPWM.ConfigureChannel(PWM::ChannelConfig(PWM::Channel::Channel_1, 50, PWM::Polarity::High));
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
    uint32_t SHORT_DELAY_MS = 2000;
    uint32_t BEEP_LONG_MS   = 1000;
    uint32_t BEEP_SHORT_MS  = 200;

    if (mButtonPressed)
    {
        bool result = false;

        mLedGreen.Set(Level::HIGH);
        mMatrix.WriteDigits(symbol_smiley);

        // Since waiting and beeping is the only function of the device, put this in
        // blocking delays. Power is not an issue, we are connected to USB.

        // Person starts to speak, wait uninterrupted
        HAL_Delay(LONG_DELAY_MS);

        // Start of beep loop
        mLedGreen.Set(Level::LOW);
        mLedOrange.Set(Level::HIGH);
        for (uint32_t i = 0; i < MAX_LOOP_COUNT; i++)
        {
            // Display digit - countdown
            uint32_t j = MAX_LOOP_COUNT - i - 1;
            switch (j) {
                case 0: { result = mMatrix.WriteDigits(digit_zero);  } break;
                case 1: { result = mMatrix.WriteDigits(digit_one);   } break;
                case 2: { result = mMatrix.WriteDigits(digit_two);   } break;
                case 3: { result = mMatrix.WriteDigits(digit_three); } break;
                case 4: { result = mMatrix.WriteDigits(digit_four);  } break;
                case 5: { result = mMatrix.WriteDigits(digit_five);  } break;
                case 6: { result = mMatrix.WriteDigits(digit_six);   } break;
                case 7: { result = mMatrix.WriteDigits(digit_seven); } break;
                case 8: { result = mMatrix.WriteDigits(digit_eight); } break;
                case 9: { result = mMatrix.WriteDigits(digit_nine);  } break;
                default: break;
            };
            EXPECT(result);

            // Short beep
            result = mPWM.Start(PWM::Channel::Channel_1);
            EXPECT(result);
            HAL_Delay(BEEP_SHORT_MS);
            result = mPWM.Stop(PWM::Channel::Channel_1);
            EXPECT(result);

            // Wait before next loop
            HAL_Delay(SHORT_DELAY_MS);

            // Make delays between loops shorter each iteration
            BEEP_SHORT_MS  += 10;
            SHORT_DELAY_MS -= 150;
        }

        // Last long beep
        mLedOrange.Set(Level::LOW);
        mLedRed.Set(Level::HIGH);
        mMatrix.WriteDigits(symbol_sadface);
        result = mPWM.Start(PWM::Channel::Channel_1);
        EXPECT(result);
        HAL_Delay(BEEP_LONG_MS);
        result = mPWM.Stop(PWM::Channel::Channel_1);
        EXPECT(result);

        // Reset counters
        SHORT_DELAY_MS = 2000;
        BEEP_LONG_MS   = 1000;
        BEEP_SHORT_MS  = 200;

        // Wait before returning to default state
        HAL_Delay(SHORT_DELAY_MS);

        // Prepare for new person
        mLedRed.Set(Level::LOW);
        mMatrix.ClearDisplay();

        mButtonPressed = false;
    }
}

/**
 * \brief   Error handler, acts as visual indicator to the user that the
 *          application entered an error state by toggling the red led.
 */
void Application::Error()
{
#if (DEBUG)
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


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Callback for the button pressed event.
 */
void Application::ButtonPressedCallback()
{
    mButtonPressed = true;
}
