/**
 * \file    AppLogic.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Interface-injected application logic for the StandupCounter.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/target/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "AppLogic.hpp"
#include "components/HI-M1388AR/HI-M1388AR_Lib.hpp"
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
 * \brief   Constructor, binds the injected driver interfaces and delay.
 * \param   matrix     8x8 LED matrix used to show the countdown and faces.
 * \param   pwm        PWM driving the buzzer beeps.
 * \param   ledGreen   Green led, lit while the speaker is talking.
 * \param   ledOrange  Orange led, lit during the beep countdown.
 * \param   ledRed     Red led, lit for the final beep.
 * \param   clock      Blocking delay seam (real HAL_Delay or a recording mock).
 */
AppLogic::AppLogic(IHI_M1388AR& matrix, IPWM& pwm, IPin& ledGreen, IPin& ledOrange, IPin& ledRed, IDelay& clock) :
    mMatrix(matrix),
    mPWM(pwm),
    mLedGreen(ledGreen),
    mLedOrange(ledOrange),
    mLedRed(ledRed),
    mClock(clock),
    mButtonPressed(false)
{ ; }

/**
 * \brief   Handler for the button-pressed event. Flags that a standup
 *          countdown should start on the next Process().
 */
void AppLogic::OnButtonPressed()
{
    mButtonPressed = true;
}

/**
 * \brief   Main process step. When a button press is pending, runs the full
 *          standup countdown choreography (smiley, long speaking wait, the
 *          beep countdown with shrinking delays, the final long beep, reset).
 *          To be called often.
 */
void AppLogic::Process()
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
        mClock.DelayMs(LONG_DELAY_MS);

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
            result = mPWM.Start(IPWM::Channel::Channel_1);
            EXPECT(result);
            mClock.DelayMs(BEEP_SHORT_MS);
            result = mPWM.Stop(IPWM::Channel::Channel_1);
            EXPECT(result);

            // Wait before next loop
            mClock.DelayMs(SHORT_DELAY_MS);

            // Make delays between loops shorter each iteration
            BEEP_SHORT_MS  += 10;
            SHORT_DELAY_MS -= 150;
        }

        // Last long beep
        mLedOrange.Set(Level::LOW);
        mLedRed.Set(Level::HIGH);
        mMatrix.WriteDigits(symbol_sadface);
        result = mPWM.Start(IPWM::Channel::Channel_1);
        EXPECT(result);
        mClock.DelayMs(BEEP_LONG_MS);
        result = mPWM.Stop(IPWM::Channel::Channel_1);
        EXPECT(result);

        // Reset counters
        SHORT_DELAY_MS = 2000;
        BEEP_LONG_MS   = 1000;
        BEEP_SHORT_MS  = 200;

        // Wait before returning to default state
        mClock.DelayMs(SHORT_DELAY_MS);

        // Prepare for new person
        mLedRed.Set(Level::LOW);
        mMatrix.ClearDisplay();

        mButtonPressed = false;
    }
}
