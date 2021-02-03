/**
 * \file Application.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for Accelerometer demo.
 *
 * \note    <ToDo: link to url on github>
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        10-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <functional>
#include "Application.hpp"
#include "board/BoardConfig.hpp"
#include "utility/heap_check.h"
#include "utility/stack_painting.h"
#include "utility/SlimAssert.h"


/************************************************************************/
/* Static Variables                                                     */
/************************************************************************/
static volatile uint32_t used_stack = 0;
static volatile uint32_t used_heap  = 0;


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
    mButtonPressed(false)
{
    mButton.Interrupt(Trigger::RISING, [this]() { this->ButtonPressedCallback(); } );
}

/**
 * \brief   Initialize the various peripherals, configures components and show
 *          the user the application is starting using the leds.
 * \returns True if init is successful, else false.
 */
bool Application::Init()
{
    bool result = true;

    mLedGreen.Set(Level::HIGH);
    
    // Simulate initialization by adding delay
    HAL_Delay(750);

    mLedGreen.Set(Level::LOW);

    return result;
}

/**
 * \brief   Main process loop of the application. This method is to be called
 *          often and acts as the main processor of data of the application.
 */
void Application::Process()
{
    if (mButtonPressed)
    {
        mButtonPressed = false;

        mLedGreen.Toggle();
    }

    mCpuWakeCounter.EnterSleepMode(SleepMode::WaitForInterrupt);

    if (mCpuWakeCounter.IsUpdated()) {
        GetUsedHeap();
        GetUsedStack();

        CpuStats stats = mCpuWakeCounter.GetStatistics();

        // Do something with the statistics - log?
        bool wakePercentageThresholdReached = (stats.wakePercentage > 90.0f);
        if (wakePercentageThresholdReached) {
            mLedRed.Set(Level::HIGH);
            ASSERT(wakePercentageThresholdReached);
        } else if (stats.wakePercentage > 20.0f) {
            if (mLedOrange.Get() != Level::HIGH) {
                mLedOrange.Set(Level::HIGH);
            }
        } else {
            if (mLedOrange.Get() != Level::LOW) {
                mLedOrange.Set(Level::LOW);
            }
        }
    }
}

/**
 * \brief   Error handler, acts as visual indicator to the user that the
 *          application entered an error state by toggling the green led.
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

/**
 * \brief   Update the max amount of stack memory used.
 */
void Application::GetUsedStack() {
    uint32_t tmp = get_used_stack();
    if (tmp > used_stack) {
        used_stack = tmp;
    }
}

/**
 * \brief   Update the max amount of heap memory used.
 */
void Application::GetUsedHeap() {
    uint32_t tmp = get_used_heap();
    if (tmp > used_heap) {
        used_heap = tmp;
    }
}
