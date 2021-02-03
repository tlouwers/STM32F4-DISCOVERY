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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/ExampleProject/target/Src
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
#include "utility/HeapCheck/heap_check.h"
#include "utility/StackPainting/stack_painting.h"
#include "utility/SlimAssert/SlimAssert.h"


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
    mChipSelect(PIN_SPI1_CS, Level::HIGH),              // SPI ChipSelect for Motion
    mMotionInt1(PIN_MOTION_INT1, PullUpDown::HIGHZ),
    mMotionInt2(PIN_MOTION_INT2, PullUpDown::HIGHZ),
    mSPI(SPIInstance::SPI_1),
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mLIS3DSH(mSPI, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2),
    mButtonPressed(false),
    mMotionDataAvailable(false),
    mMotionLength(0)
{
    mButton.Interrupt(Trigger::RISING, [this]() { this->ButtonPressedCallback(); } );
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
    HAL_Delay(750);

    // Actual Init()
    mButton.Interrupt(Trigger::RISING, [this]() { this->ButtonPressedCallback(); } );
    mButtonPressed = false;

    bool result = mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Tx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaTxHandle());
    ASSERT(result);

    result = mDMA_SPI_Rx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaRxHandle());
    ASSERT(result);

    result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);

    result = mLIS3DSH.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz));
    ASSERT(result);
    mMotionDataAvailable = false;
    mMotionLength = 0;

    result = mLIS3DSH.Enable();
    ASSERT(result);

    mLedGreen.Set(Level::LOW);

    return result;
}

/**
 * \brief   Main process loop of the application. This method is to be called
 *          often and acts as the main processor of data of the application.
 */
void Application::Process()
{
    static uint8_t motionArray[25 * 3 * 2] = {};

    if (mButtonPressed)
    {
        mButtonPressed = false;

        mLedGreen.Set(Level::LOW);
    }

    if (mMotionDataAvailable)
    {
        mMotionDataAvailable = false;

        bool retrieveResult = mLIS3DSH.RetrieveAxesData(motionArray, mMotionLength);
        ASSERT(retrieveResult);
		(void)(retrieveResult);

        // Deinterleave to X,Y,Z samples
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
    mLedGreen.Set(Level::HIGH);
}

/**
 * \brief   Callback called for the motion data received callback.
 */
void Application::MotionDataReceived(uint8_t length)
{
    mLedOrange.Toggle();

    mMotionDataAvailable = true;
    mMotionLength = length;
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
