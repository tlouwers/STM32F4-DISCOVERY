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
#include "utility/SlimAssert/SlimAssert.h"


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
    mChipSelect(PIN_SPI2_CS, Level::HIGH),              // SPI ChipSelect
    mPWMOut(PIN_PWM_CH1, Alternate::AF1),
    mPWM(PwmTimerInstance::TIMER_2),
    mSPI(SPIInstance::SPI_2),
    mDMA_SPI_Tx(DMA::Stream::Dma1_Stream4),
    mDMA_SPI_Rx(DMA::Stream::Dma1_Stream3),
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
    bool result = mDMA_SPI_Tx.Configure(DMA::Channel::Channel0, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Rx.Configure(DMA::Channel::Channel0, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::DataWidth::Byte, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    ASSERT(result);

    result = mDMA_SPI_Tx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaTxHandle());
    ASSERT(result);

    result = mDMA_SPI_Rx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaRxHandle());
    ASSERT(result);

    result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    ASSERT(result);


    result = mPWM.Init(PWM::Config(500));
    ASSERT(result);

    result = mPWM.ConfigureChannel(PWM::ChannelConfig(PWM::Channel::Channel_1, 50, PWM::Polarity::High));
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
    static bool pwm_on = false;

    if (mButtonPressed)
    {
        mButtonPressed = false;

        mLedGreen.Set(Level::LOW);

        if (pwm_on) {
            pwm_on = false;
            mPWM.Stop(PWM::Channel::Channel_1);
        } else {
            pwm_on = true;
            mPWM.Start(PWM::Channel::Channel_1);
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
