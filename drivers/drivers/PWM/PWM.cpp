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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/PWM
 *
 * \details Intended use is to provide an easier means to work with PWM
 *          channels. For this driver it is hardcoded to timer 2, all 4 channels
 *          can be used.
 *          Can be ported easily to timer 3, 4 or 5 as well, since most features
 *          are generic. This is something for a future update.
 *          Hardcoded items:
 *          Using timer 2, which is using APB1 timer clock as clock source.
 *
 * \note    Inspiration from:
 *          https://stm32f4-discovery.net/2014/05/stm32f4-stm32f429-discovery-pwm-tutorial/
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
/**
 * \brief   Constructor, prepares the PWM for use.
 */
PWM::PWM() :
    mInitialized(false)
{
    ASSERT(!ptrToPwmInstance);
    ptrToPwmInstance = this;
}

/**
 * \brief   Destructor, stops PWM output.
 */
PWM::~PWM()
{
    Sleep();

    ptrToPwmInstance = nullptr;
}

/**
 * \brief   Initializes the PWM class using Timer 2 as clock source.
 *          Sets the PWM frequency.
 * \param   config  The configuration for the PWM instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool PWM::Init(const Config& config)
{
    ASSERT(config.mFrequency > 0);
    ASSERT(config.mFrequency <= UINT16_MAX);

    if (config.mFrequency == 0) { return false; }

    CheckAndEnableAPB1TimerClock();

    // The timer tick frequency is set with: timer_tick_frequency = timer_default_frequency / (prescaler + 1)
    // We use the max frequency for the timer: set prescaler to 0 and the timer will have the max tick frequency.
    // timer_tick_frequency = 8000000 / (0 + 1) = 8000000 Hz

    // Given the desired PWM_frequency, we calculate the timer_period:
    // timer_period = timer_tick_frequency / PWM_frequency - 1

    uint32_t timer_period = 8000000 / (config.mFrequency - 1);

    // Check if the timer_period is within valid range: UINT16_MAX. For timer 2 and 5 this can be 32 bit,
    // but as preperation for future use with timer 3 and 4 we use 16 bit max.
    // If the value is too large we can use the prescaler.
    if ((timer_period == 0) || (timer_period > UINT16_MAX)) { return false; }

    // Start timer 2 as clock for PWM. No channels are configured yet.
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

/**
 * \brief    Puts the PWM module in sleep mode.
 *           Stops PWM output on all channels.
 */
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

    DisableAPB1TimerClock();

    return result;
}

/**
 * \brief   Configure PWM output for a channel. The duty cycle and ON polarity
 *          can be configured. This does NOT start PWM output.
 * \param   channelConfig   The configuration for a PWM channel.
 * \result  True if the configuration could be applied, else false.
 */
bool PWM::ConfigureChannel(const ChannelConfig& channelConfig)
{
    if (!mInitialized) { return false; }

    ASSERT(channelConfig.mDutyCycle <= 100);

    // We calculate the pulse_length by using the given duty cycle - which here is in percent [0..100%]
    // pulse_length = (((timer_period + 1) * duty cycle) / 100) - 1

    uint32_t pulse_length = (((mHandle.Init.Period + 1) * channelConfig.mDutyCycle) / 100) - 1;

    // Remember: if pulse_length is larger than timer_period, you will have output HIGH all the time

    TIM_OC_InitTypeDef ocInit = {};

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

/**
 * \brief   Start PWM output for a channel (if configured first).
 * \param   channel     The channel to start PWM output for.
 * \result  True if the PWM output could be started for the given channel, else false.
 */
bool PWM::Start(Channel channel)
{
    if (!mInitialized) { return false; }

    if (HAL_TIM_PWM_Start(&mHandle, GetChannel(channel)) == HAL_OK)
    {
        return true;
    }
    return false;
}

/**
 * \brief   Stop PWM output for a channel (if configured first).
 * \param   channel     The channel to stop PWM output for.
 * \result  True if the PWM output could be stopped for the given channel, else false.
 */
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
/**
 * \brief   Check if the APB1 timer clock for Timer 2 is started, else enable it.
 */
void PWM::CheckAndEnableAPB1TimerClock()
{
    if (__HAL_RCC_TIM2_IS_CLK_DISABLED()) { __HAL_RCC_TIM2_CLK_ENABLE(); }
}

/**
 * \brief   Stop the APB1 timer clock for Timer 2.
 */
void PWM::DisableAPB1TimerClock()
{
    __HAL_RCC_TIM2_CLK_DISABLE();
}

/**
 * \brief   Get the TIM_Channel define using the given channel.
 * \param   The channel to get the TIM_Channel define for.
 * \returns The TIM_Channel if successful, else 0.
 */
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

/**
 * \brief   Stop PWM output for all available channels.
 * \returns True if PWM output for all channels could be stopped, else false.
 */
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
