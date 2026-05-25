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
 * \date    10-2019
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
    mLedGreen(PIN_LED_GREEN, Level::LOW),               // Off
    mLedOrange(PIN_LED_ORANGE, Level::LOW),
    mLedRed(PIN_LED_RED, Level::LOW),
    mLedBlue(PIN_LED_BLUE, Level::LOW),
    mChipSelect(PIN_SPI1_CS, Level::HIGH),              // SPI ChipSelect for Motion
    mMotionInt1(PIN_MOTION_INT1, PullUpDown::HIGHZ),
    mMotionInt2(PIN_MOTION_INT2, PullUpDown::HIGHZ),
    mTim1(GenericTimerInstance::TIMER_10),
    mTim2(GenericTimerInstance::TIMER_11),
    mTim3(GenericTimerInstance::TIMER_12),
    mSPI(SPIInstance::SPI_1),
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mLIS3DSH(mSPI, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2),
    mLogic(mLIS3DSH, mWatchdog, mLedOrange)
{
    // Note: button conflicts with the accelerometer int1 pin. This is a board layout issue.
    mLIS3DSH.SetHandler( [this](uint8_t length) { mLogic.OnMotionData(length); } );
}

/**
 * \brief   Initialize the various peripherals, configures components and show
 *          the user the application is starting using the leds.
 * \returns True if init is successful, else false.
 */
bool Application::Init()
{
    mLedGreen.Set(Level::HIGH);

    // Simulate initialization by adding delay
    HAL_Delay(750);

    // Actual Init()
    bool result = mCpuWakeCounter.Init();
    ASSERT(result);

    result = mWatchdog.Init(Watchdog::Config(Watchdog::Timeout::_4_S));     // 4 seconds
    ASSERT(result);

    result = mTim1.Init(GenericTimer::Config(15, 5.00f));                   // 5.00 Hz --> 200 ms
    ASSERT(result);

    result = mTim2.Init(GenericTimer::Config(16, 2.22f));                   // 2.22 Hz --> 450 ms
    ASSERT(result);

    result = mTim3.Init(GenericTimer::Config(17, 1.74f));                   // 1.74 Hz --> 575 ms
    ASSERT(result);

    result = mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    // SPI infers Tx/Rx from each DMA's configured Direction; full-duplex
    // links both. Must follow Configure() (LinkDma checks IsConfigured()).
    result = mSPI.LinkDma(mDMA_SPI_Tx);
    ASSERT(result);

    result = mSPI.LinkDma(mDMA_SPI_Rx);
    ASSERT(result);

    result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);

    result = mLIS3DSH.Init(LIS3DSH::Config(mLIS3DSHBuf, sizeof(mLIS3DSHBuf), true, LIS3DSH::SampleFrequency::_50_Hz));
    ASSERT(result);

    mTim1.Start([this]() { this->CallbackLedGreenToggle(); });
    mTim2.Start([this]() { this->CallbackLedRedToggle();   });
    mTim3.Start([this]() { this->CallbackLedBlueToggle();  });

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
    mLogic.Process();

    // Handle an update (if available)
    if (mCpuWakeCounter.IsUpdated())    // Will update once per second
    {
        // Get the updated statistics
        CpuStats cpuStats = mCpuWakeCounter.GetStatistics();

        mLogic.ServiceTick(cpuStats.wakePercentage);
    }

    // At the end of the main process loop enter the desired sleep mode
    mCpuWakeCounter.EnterSleepMode(SleepMode::WaitForInterrupt);
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


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
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
