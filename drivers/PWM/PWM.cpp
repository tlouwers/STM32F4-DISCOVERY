/**
 * \file PWM.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   PWM class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/PWM
 *
 * \details Intended use is to provide an easier means to work with PWM
 *          channels. For this driver it is hardcoded to timer 2, all 4 channels
 *          can be used.
 *
 *          As example:
 *
 *          // Declare the class (in Application.hpp for example):
 *          Usart mUsart;
 *
 *          // Construct the class, indicate the instance to use:
 *          Application::Application() :
 *              mUsart(UsartInstance::USART_2)
 *          {}
 *
 *          // To Write (interrupt based):
 *          uint8_t write_buffer[] = "test\r\n";
 *          bool result = mUsart.WriteInterrupt(write_buffer, sizeof(write_buffer), [this]() { this->WriteDone(); } );
 *          assert(result);
 *
 *          // To Read (interrupt based):
 *          uint8_t read_buffer[6] = {0};
 *          result = mUsart.ReadInterrupt(read_buffer, sizeof(read_buffer), [this](uint16_t bytesReceived) { this->ReadDone(bytesReceived); });
 *          assert(result);
 *
 *          // The ReadDone callback (as example):
 *          void Application::ReadDone(uint16_t bytesReceived)
 *          {
 *              if (bytesReceived > 0)
 *              {
 *                  // Do stuff ...
 *              }
 *          }
 *      Inspiration and some formulas from:
 *      https://stm32f4-discovery.net/2014/05/stm32f4-stm32f429-discovery-pwm-tutorial/
 *
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/PWM/PWM.hpp"
#include "utility/SlimAssert.h"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Static variable initialization                                       */
/************************************************************************/
static PWM* ptrToPwmInstance = nullptr;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
PWM::PWM() :
    mInitialized(false)
{
    ASSERT(!ptrToPwmInstance);
    ptrToPwmInstance = this;
}

PWM::~PWM()
{
    Sleep();

    ptrToPwmInstance = nullptr;
}

bool PWM::Init(const Config& config)
{
    if (config.mFrequency == 0) { return false; }

    CheckAndEnableAHB1PeripheralClock();

    // Timer count frequency is set with: timer_tick_frequency = timer_default_frequency / (prescaler + 1)
    // Use max frequency for timer: set prescaler to 0 and timer will have tick frequency
    // timer_tick_frequency = 8000000 / (0 + 1) = 8000000 Hz

    // PWM_frequency = timer_tick_frequency / (timer_period + 1)
    // timer_period = timer_tick_frequency / PWM_frequency - 1

    uint32_t timer_period = 8000000 / (config.mFrequency - 1);

    if ((timer_period == 0) || (timer_period > UINT16_MAX)) { return false; }

    mHandle.Instance           = TIM2;
    mHandle.Init.Prescaler     = 0;
    mHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    mHandle.Init.Period        = timer_period;
    mHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    if (HAL_TIM_PWM_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

bool PWM::Sleep()
{
    bool result = true;

    mInitialized = false;

    result &= StopAllChannels();
    ASSERT(result);

    if (HAL_TIM_PWM_DeInit(&mHandle) != HAL_OK)
    {
        result = false;
    }
    ASSERT(result);

    DisableAHB1PeripheralClock();

    return result;
}

bool PWM::ConfigureChannel(const ChannelConfig& channelConfig)
{
    if (!mInitialized) { return false; }

    ASSERT(channelConfig.mDutyCycle <= 100);

    TIM_OC_InitTypeDef ocInit = {};

    // Remember: if pulse_length is larger than timer_period, you will have output HIGH all the time
    // Note: DutyCycle is in percent, between 0 and 100%
    // pulse_length = (((timer_period + 1) * DutyCycle) / 100) - 1

    uint32_t pulse_length = (((mHandle.Init.Period + 1) * channelConfig.mDutyCycle) / 100) - 1;

    ocInit.OCMode     = TIM_OCMODE_PWM2;    // Clear on compare match
    ocInit.Pulse      = pulse_length;
    ocInit.OCPolarity = (channelConfig.mPolarity == Polarity::High) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    ocInit.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&mHandle, &ocInit, GetChannel(channelConfig.mChannel)) == HAL_OK)
    {
        return true;
    }
    return false;
}

bool PWM::Start(Channel channel)
{
    if (!mInitialized) { return false; }

    if (HAL_TIM_PWM_Start(&mHandle, GetChannel(channel)) == HAL_OK)
    {
        return true;
    }
    return false;
}

bool PWM::Stop(Channel channel)
{
    if (!mInitialized) { return false; }

    if (HAL_TIM_PWM_Stop(&mHandle, GetChannel(channel)) == HAL_OK)
    {
        return true;
    }
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
void PWM::CheckAndEnableAHB1PeripheralClock()
{
    // TIM2 is connected to APB1 bus, which has on F407 device 42MHz clock.
    // But, timer has internal PLL, which double this frequency for timer, up to 84MHz.

    if (__HAL_RCC_TIM2_IS_CLK_DISABLED()) { __HAL_RCC_TIM2_CLK_ENABLE(); }
}

void PWM::DisableAHB1PeripheralClock()
{
    __HAL_RCC_TIM2_CLK_DISABLE();
}

uint32_t PWM::GetChannel(Channel channel)
{
    uint32_t channelId = 0;

    switch (channel)
    {
        case Channel::Channel_1: channelId = TIM_CHANNEL_1; break;
        case Channel::Channel_2: channelId = TIM_CHANNEL_2; break;
        case Channel::Channel_3: channelId = TIM_CHANNEL_3; break;
        case Channel::Channel_4: channelId = TIM_CHANNEL_4; break;
        default: ASSERT(false); break;      // Impossible selection
    }

    return channelId;
}

bool PWM::StopAllChannels()
{
    bool result = true;

    result &= Stop(Channel::Channel_1);
    ASSERT(result);
    result &= Stop(Channel::Channel_2);
    ASSERT(result);
    result &= Stop(Channel::Channel_3);
    ASSERT(result);
    result &= Stop(Channel::Channel_4);
    ASSERT(result);

    return result;
}
