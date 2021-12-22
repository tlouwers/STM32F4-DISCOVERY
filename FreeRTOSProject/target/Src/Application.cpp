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
 * \brief   Main application file for Accelerometer demo.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/ExampleProject/target/Src
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    12-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <functional>
#include "Application.hpp"
#include "board/BoardConfig.hpp"
#include "utility/Assert/Assert.h"
#include "../FreeRTOS/include/FreeRTOS.h"
#include "../FreeRTOS/include/task.h"


/************************************************************************/
/* Task - definitions                                                   */
/************************************************************************/
void vBlinkLedGreen(void* pvParam);
void vBlinkLedOrange(void* pvParam);
void vBlinkLedRed(void* pvParam);
void vBlinkLedBlue(void* pvParam);

static std::function<void()> callbackLedGreenToggle  = nullptr;
static std::function<void()> callbackLedOrangeToggle = nullptr;
static std::function<void()> callbackLedRedToggle    = nullptr;
static std::function<void()> callbackLedBlueToggle   = nullptr;


/************************************************************************/
/* Static Functions                                                     */
/************************************************************************/
static void CallbackLedGreenToggle()
{
    if (callbackLedGreenToggle) { callbackLedGreenToggle(); }
}

static void CallbackLedOrangeToggle()
{
    if (callbackLedOrangeToggle) { callbackLedOrangeToggle(); }
}

static void CallbackLedRedToggle()
{
    if (callbackLedRedToggle) { callbackLedRedToggle(); }
}

static void CallbackLedBlueToggle()
{
    if (callbackLedBlueToggle) { callbackLedBlueToggle(); }
}


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
    mShouldBlinkLeds(false)
{
    mButton.Interrupt(Trigger::RISING, [this]() { this->CallbackButtonPressed(); } );
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
    
    // Connect callbacks (C to C++ bridge)
    callbackLedGreenToggle  = [this]() { this->CallbackLedGreenToggle();  };
    callbackLedOrangeToggle = [this]() { this->CallbackLedOrangeToggle(); };
    callbackLedRedToggle    = [this]() { this->CallbackLedRedToggle();    };
    callbackLedBlueToggle   = [this]() { this->CallbackLedBlueToggle();   };


    // Simulate initialization by adding delay
    HAL_Delay(750);

    mLedGreen.Set(Level::LOW);

    return result;
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

/**
 * \brief   Create the various FreeRTOS tasks to run the system with.
 */
bool Application::CreateTasks()
{
    bool result = false;

    result = ( xTaskCreate( vBlinkLedGreen,  "Blink Green Task",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vBlinkLedOrange, "Blink Orange Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vBlinkLedRed,    "Blink Red Task",    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vBlinkLedBlue,   "Blink Blue Task",   configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);

    return result;
}

/**
 * \brief   Start the FreeRTOS scheduler, start tasks.
 * \note    This replaces the main loop.
 */
void Application::StartTasks()
{
    vTaskStartScheduler();
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Callback for the button pressed event.
 */
void Application::CallbackButtonPressed()
{
    mShouldBlinkLeds = !mShouldBlinkLeds;

    if (!mShouldBlinkLeds)
    {
        mLedGreen.Set(Level::LOW);
        mLedOrange.Set(Level::LOW);
        mLedRed.Set(Level::LOW);
        mLedBlue.Set(Level::LOW);
    }
}

/**
 * \brief   Callback for the green led toggle event.
 */
void Application::CallbackLedGreenToggle()
{
    if (mShouldBlinkLeds) { mLedGreen.Toggle(); }
}

/**
 * \brief   Callback for the orange led toggle event.
 */
void Application::CallbackLedOrangeToggle()
{
    if (mShouldBlinkLeds) { mLedOrange.Toggle(); }
}

/**
 * \brief   Callback for the red led toggle event.
 */
void Application::CallbackLedRedToggle()
{
    if (mShouldBlinkLeds) { mLedRed.Toggle(); }
}

/**
 * \brief   Callback for the blue led toggle event.
 */
void Application::CallbackLedBlueToggle()
{
    if (mShouldBlinkLeds) { mLedBlue.Toggle(); }
}


/************************************************************************/
/* Tasks                                                                */
/************************************************************************/
/**
 * \brief   Blink led green task handler.
 * \details Configured to be executed every 200 milliseconds.
 */
void vBlinkLedGreen(void *pvParameters)
{
    while (true)
    {
        CallbackLedGreenToggle();
        vTaskDelay( 200 / portTICK_RATE_MS );
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Blink led orange task handler.
 * \details Configured to be executed every 300 milliseconds.
 */
void vBlinkLedOrange(void *pvParameters)
{
    while (true)
    {
        CallbackLedOrangeToggle();
        vTaskDelay( 300 / portTICK_RATE_MS );
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Blink led red task handler.
 * \details Configured to be executed every 450 milliseconds.
 */
void vBlinkLedRed(void *pvParameters)
{
    while (true)
    {
        CallbackLedRedToggle();
        vTaskDelay( 450 / portTICK_RATE_MS );
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Blink led blue task handler.
 * \details Configured to be executed every 575 milliseconds.
 */
void vBlinkLedBlue(void *pvParameters)
{
    while (true)
    {
        CallbackLedBlueToggle();
        vTaskDelay( 575 / portTICK_RATE_MS );
    }

    vTaskDelete( NULL );
}
