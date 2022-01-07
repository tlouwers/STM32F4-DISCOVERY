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
void vBlinkLedRed(void* pvParam);
void vBlinkLedBlue(void* pvParam);
void vMotionData(void* pvParam);
static TaskHandle_t xMotionData = NULL;

static std::function<void()> callbackLedGreenToggle     = nullptr;
static std::function<void()> callbackLedRedToggle       = nullptr;
static std::function<void()> callbackLedBlueToggle      = nullptr;
static std::function<void()> callbackMotionDataReceived = nullptr;


/************************************************************************/
/* Static Functions                                                     */
/************************************************************************/
static void CallbackLedGreenToggle()
{
    if (callbackLedGreenToggle) { callbackLedGreenToggle(); }
}

static void CallbackLedRedToggle()
{
    if (callbackLedRedToggle) { callbackLedRedToggle(); }
}

static void CallbackLedBlueToggle()
{
    if (callbackLedBlueToggle) { callbackLedBlueToggle(); }
}

static void CallbackMotionDataReceived()
{
    if (callbackMotionDataReceived) { callbackMotionDataReceived(); }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, configures pins and callbacks.
 */
Application::Application() :
    mLedGreen(PIN_LED_GREEN, Level::LOW),               // Off
    mLedOrange(PIN_LED_ORANGE, Level::LOW),
    mLedRed(PIN_LED_RED, Level::LOW),
    mLedBlue(PIN_LED_BLUE, Level::LOW),
    mChipSelect(PIN_SPI1_CS, Level::HIGH),              // SPI ChipSelect for Motion
    mMotionInt1(PIN_MOTION_INT1, PullUpDown::HIGHZ),
    mMotionInt2(PIN_MOTION_INT2, PullUpDown::HIGHZ),
    mSPI(SPIInstance::SPI_1),
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mLIS3DSH(mSPI, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2),
    mMotionLength(0)
{
    // Note: button conflicts with the accelerometer int1 pin. This is a board layout issue.
    mLIS3DSH.SetHandler( [this](uint8_t length) { this->MotionDataReceived(length); } );
}

/**
 * \brief   Initialize the various peripherals, configures components and show
 *          the user the application is starting using the leds.
 * \returns True if init is successful, else false.
 */
bool Application::Init()
{
    mLedGreen.Set(Level::HIGH);

    // Connect callbacks (C to C++ bridge)
    callbackLedGreenToggle     = [this]() { this->CallbackLedGreenToggle();     };
    callbackLedRedToggle       = [this]() { this->CallbackLedRedToggle();       };
    callbackLedBlueToggle      = [this]() { this->CallbackLedBlueToggle();      };
    callbackMotionDataReceived = [this]() { this->CallbackMotionDataReceived(); };


    // Simulate initialization by adding delay
    HAL_Delay(750);

    // Actual Init()
    bool result = mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Tx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaTxHandle());
    ASSERT(result);

    result = mDMA_SPI_Rx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaRxHandle());
    ASSERT(result);

    result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);

    result = mLIS3DSH.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz));
    ASSERT(result);
    mMotionLength = 0;

    result = mLIS3DSH.Enable();
    ASSERT(result);

    mLedGreen.Set(Level::LOW);

    return result;
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

/**
 * \brief   Create the various FreeRTOS tasks to run the system with.
 */
bool Application::CreateTasks()
{
    bool result = false;

    result = ( xTaskCreate( vBlinkLedGreen,  "Blink Green Task",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vBlinkLedRed,    "Blink Red Task",    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vBlinkLedBlue,   "Blink Blue Task",   configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vMotionData,     "Motion Data Task",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xMotionData) == pdPASS ) ? true : false;
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
 * \brief   Callback called for the motion data received callback.
 */
void Application::MotionDataReceived(uint8_t length)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    mMotionLength = length;

    vTaskNotifyGiveFromISR( xMotionData, &xHigherPriorityTaskWoken );

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
 * \brief   Callback for the green led toggle event.
 */
void Application::CallbackLedGreenToggle()
{
    mLedGreen.Toggle();
}

/**
 * \brief   Callback for the red led toggle event.
 */
void Application::CallbackLedRedToggle()
{
    mLedRed.Toggle();
}

/**
 * \brief   Callback for the blue led toggle event.
 */
void Application::CallbackLedBlueToggle()
{
    mLedBlue.Toggle();
}

/**
 * \brief   Callback for the motion data received event.
 */
void Application::CallbackMotionDataReceived()
{
    static uint8_t motionArray[25 * 3 * 2] = {};

    if (mMotionLength > 0)
    {
        mLedOrange.Toggle();

        bool retrieveResult = mLIS3DSH.RetrieveAxesData(motionArray, mMotionLength);
        EXPECT(retrieveResult);
        (void)(retrieveResult);

        // Deinterleave to X,Y,Z samples
    }
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

/**
 * \brief   Handle motion data as it arrives per callback (via ISR context).
 */
void vMotionData(void *pvParameters)
{
    const TickType_t xBlockTime = pdMS_TO_TICKS( 500 );
    uint32_t ulNotifiedValue;

    while (true)
    {
        ulNotifiedValue = ulTaskNotifyTake( pdFALSE, xBlockTime );

        if (ulNotifiedValue > 0)
        {
            CallbackMotionDataReceived();
        }
    }

    vTaskDelete( NULL );
}
