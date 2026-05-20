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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/TiltExample/target/Src
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
#include <math.h>
#include <functional>
#include "Application.hpp"
#include "board/BoardConfig.hpp"
#include "utility/Assert/Assert.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static const uint16_t MOTION_SAMPLE_SIZE = 3 * 2;   // X,Y,Z, each 16 bit signed int


/************************************************************************/
/* Task - definitions                                                   */
/************************************************************************/
void vMotionData(void* pvParam);
void vMatrix(void* pvParam);
void vUsart(void* pvParam);

static TaskHandle_t xMotionData = NULL;

static std::function<void()> callbackMotionDataReceived                              = nullptr;
static std::function<void(const MotionSample &sample)> callbackUpdateDisplay         = nullptr;
static std::function<void(const MotionSampleRaw &sample)> callbackSendSampleViaUsart = nullptr;

static QueueHandle_t displayQueue = nullptr;
static QueueHandle_t usartQueue   = nullptr;


/************************************************************************/
/* Static Functions                                                     */
/************************************************************************/
static void CallbackMotionDataReceived()
{
    if (callbackMotionDataReceived) { callbackMotionDataReceived(); }
}

static void CallbackUpdateDisplay(const MotionSample &sample)
{
    if (callbackUpdateDisplay) { callbackUpdateDisplay(sample); }
}

static void CallbackSendSampleViaUsart(const MotionSampleRaw &sample)
{
    if (callbackSendSampleViaUsart) { callbackSendSampleViaUsart(sample); }
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
    mChipSelectMatrix(PIN_SPI2_CS, Alternate::AF5),     // SPI ChipSelect for HI-M1388AR 8x8 led matrix display
    mChipSelectMotion(PIN_SPI1_CS, Level::HIGH),        // SPI ChipSelect for Motion
    mMotionInt1(PIN_MOTION_INT1, PullUpDown::HIGHZ),
    mMotionInt2(PIN_MOTION_INT2, PullUpDown::HIGHZ),
    mSPIMotion(SPIInstance::SPI_1),
    mSPIMatrix(SPIInstance::SPI_2),
    mUsart(UsartInstance::USART_2),
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mMatrix(mSPIMatrix, PIN_SPI2_CS),
    mLIS3DSH(mSPIMotion, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2),
    mLogic(mLIS3DSH, mLedOrange, mMatrix)
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
    callbackMotionDataReceived = [this]()                              { this->CallbackMotionDataReceived();       };
    callbackUpdateDisplay      = [this](const MotionSample &sample)    { this->CallbackUpdateDisplay(sample);      };
    callbackSendSampleViaUsart = [this](const MotionSampleRaw &sample) { this->CallbackSendSampleViaUsart(sample); };

    // Actual Init()
    bool result = mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    // SPI infers Tx/Rx from each DMA's configured Direction; full-duplex
    // links both. Must follow Configure() (LinkDma checks IsConfigured()).
    result = mSPIMotion.LinkDma(mDMA_SPI_Tx);
    ASSERT(result);

    result = mSPIMotion.LinkDma(mDMA_SPI_Rx);
    ASSERT(result);

    result = mSPIMotion.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);

    result = mLIS3DSH.Init(LIS3DSH::Config(mLIS3DSHBuf, sizeof(mLIS3DSHBuf), false, LIS3DSH::SampleFrequency::_50_Hz));
    ASSERT(result);


    result = mSPIMatrix.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);

    result &= mMatrix.Init(HI_M1388AR::Config(8));
    ASSERT(result);


    result = mUsart.Init(USART::Config(10, false, USART::Baudrate::_115K2));
    ASSERT(result);


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
    __disable_irq();

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

/**
 * \brief   Create the various FreeRTOS tasks to run the system with.
 */
bool Application::CreateTasks()
{
    bool result = false;

    result = ( xTaskCreate( vMotionData,     "Motion Data Task",  300,                      NULL, tskIDLE_PRIORITY + 1, &xMotionData) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vMatrix,         "Matrix Task",       300,                      NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS ) ? true : false;
    ASSERT(result);
    result = ( xTaskCreate( vUsart,          "Usart Task",        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS ) ? true : false;
    ASSERT(result);

    displayQueue = xQueueCreate( 1, sizeof(MotionSample) );
    ASSERT(displayQueue);
    usartQueue   = xQueueCreate( 1, sizeof(MotionSampleRaw) );
    ASSERT(usartQueue);

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
 * \note    This is from ISR context.
 */
void Application::MotionDataReceived(uint8_t length)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    mLogic.OnMotionData(length);

    vTaskNotifyGiveIndexedFromISR( xMotionData, 0, &xHigherPriorityTaskWoken );

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
 * \brief   Callback for the motion data received event.
 */
void Application::CallbackMotionDataReceived()
{
    static uint8_t motionArray[MOTION_SAMPLE_SIZE] = {};

    uint8_t length = 0;
    if (mLogic.RetrieveMotion(motionArray, length))
    {
        // Deinterleave to X,Y,Z samples
        for (size_t i = 0; ((i < sizeof(motionArray)) && (i + MOTION_SAMPLE_SIZE <= length)); i += MOTION_SAMPLE_SIZE)
        {
            MotionSampleRaw sampleRaw;
            sampleRaw.X = (motionArray[i + 1] << 8) | motionArray[i + 0];
            sampleRaw.Y = (motionArray[i + 3] << 8) | motionArray[i + 2];
            sampleRaw.Z = (motionArray[i + 5] << 8) | motionArray[i + 4];

            MotionSample sample = mLogic.CalculateMotionSample(sampleRaw);

            // Put converted samples to queue(s)
            BaseType_t result = xQueueSend(displayQueue, &sample, 0);
            EXPECT(result == pdPASS);
            result = xQueueSend(usartQueue, &sampleRaw, 0);
            EXPECT(result == pdPASS);
        }
    }
}

/**
 * \brief   Callback for the update display event.
 */
void Application::CallbackUpdateDisplay(const MotionSample &sample)
{
    mLogic.RenderTilt(sample);
}

/**
 * \brief   Callback for the send via Usart event.
 */
void Application::CallbackSendSampleViaUsart(const MotionSampleRaw &sample)
{
    mLedBlue.Set(Level::HIGH);

    // Wrap the sample in '<' SAMPLE '>' packet format to allow receiving side
    // to unpack it properly.
    uint8_t packet[8] = {};
    packet[0] = '<';
    packet[7] = '>';
    std::memcpy(&packet[1], reinterpret_cast<const uint8_t*>(&sample), MOTION_SAMPLE_SIZE);
    bool result = mUsart.WriteBlocking(packet, sizeof(packet));
    EXPECT(result);

    mLedBlue.Set(Level::LOW);
}


/************************************************************************/
/* Tasks                                                                */
/************************************************************************/
/**
 * \brief   Handle motion data in task after being notified from ISR (callback).
 */
void vMotionData(void *pvParameters)
{
    (void)(pvParameters);

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 500 );
    uint32_t ulNotificationValue;

    while (true)
    {
        ulNotificationValue = ulTaskNotifyTakeIndexed( 0, pdFALSE, xMaxBlockTime );

        if (ulNotificationValue == 1)
        {
            CallbackMotionDataReceived();
        }
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Matrix display handler task.
 * \details Calculates which pixel to light up and sets this on the matrix display.
 */
void vMatrix(void *pvParameters)
{
    (void)(pvParameters);

    while (true)
    {
        MotionSample sample;
        if ( xQueueReceive(displayQueue, &sample, portMAX_DELAY) == pdPASS )
        {
            CallbackUpdateDisplay(sample);
        }
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Usart task.
 * \details Sends raw motion data towards PC.
 */
void vUsart(void *pvParameters)
{
    (void)(pvParameters);

    while (true)
    {
        // While data in queue: send to Usart (towards PC)
        MotionSampleRaw sample;
        if ( xQueueReceive(usartQueue, &sample, portMAX_DELAY) == pdPASS )
        {
            CallbackSendSampleViaUsart(sample);
        }
    }

    vTaskDelete( NULL );
}

/**
 * \brief   Application Idle Hook, implements a light sleep mode.
 */
extern "C" void vApplicationIdleHook(void)
{
    __WFI();
}

/**
 * \brief   Application StackOverflow Hook, implements assert when FreeRTOS task stack overflows.
 * \param   xTask       Handle to the task which has a task overflow.
 * \param   pcTaskName  Name of the task.
 */
extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ) {
    (void)(xTask);
    (void)(pcTaskName);

    ASSERT(false);
}
