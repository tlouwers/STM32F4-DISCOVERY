/**
 * \file    PWM.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class using Timer2..4 to provide PWM functionality.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/PWM
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/PWM/PWM.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the PWM for use.
 */
PWM::PWM(const PwmTimerInstance& instance) :
    mInstance(instance),
    mInitialized(false)
{
    SetInstance(instance);
}

/**
 * \brief   Destructor, stops PWM output.
 */
PWM::~PWM()
{
    Sleep();
}

/**
 * \brief   Initializes the PWM timer instance with the given configuration.
 *          It does not configure the channel(s).
 * \param   config  The configuration for the PWM instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool PWM::Init(const IConfig& config)
{
    CheckAndEnablePeripheralClock(mInstance);

    EXPECT(config.ConfigId() == Config::Id());
    if (config.ConfigId() != Config::Id()) { return false; }

    const Config& cfg = static_cast<const Config&>(config);

    // Start the timer as clock for PWM. No channels are configured yet.
    // Prescaler stays at 0 (divide-by-1) so CK_CNT is the full timer input
    // clock; CalculatePeriod reads that input clock dynamically.
    mHandle.Init.Prescaler         = 0;
    mHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    mHandle.Init.Period            = CalculatePeriod(cfg.mFrequency);    // (Freq. desired) = (Freq. CK_CNT) / (TIMx_ARR + 1)
    mHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    mHandle.Init.RepetitionCounter = 0;
    mHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if PWM is initialized.
 * \returns True if PWM is initialized, else false.
 */
bool PWM::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the PWM module in sleep mode.
 * \details Stops PWM output on all channels.
 * \returns True if the PWM module could be put in sleep mode, else false.
 */
bool PWM::Sleep()
{
    StopAllChannels();

    if (HAL_TIM_PWM_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    CheckAndDisablePeripheralClock(mInstance);
    return true;
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

    TIM_OC_InitTypeDef ocInit = {};

    ocInit.OCMode     = TIM_OCMODE_PWM2;    // Clear on compare match
    ocInit.Pulse      = CalculatePulse(channelConfig.mDutyCycle, mHandle.Init.Period);
    ocInit.OCPolarity = (channelConfig.mPolarity == Polarity::High) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    ocInit.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&mHandle, &ocInit, GetChannel(channelConfig.mChannel)) == HAL_OK)
    {
        return true;
    }
    return false;
}

/**
 * \brief   Update only the duty cycle of an already-configured channel.
 * \param   channel     The PWM channel to update.
 * \param   dutyCycle   New duty cycle as a fraction of the period, [0.0 .. 1.0].
 * \returns True if the duty cycle was applied, false if PWM is not initialised.
 * \details Writes only the channel's CCR via __HAL_TIM_SET_COMPARE -- a single
 *          MMIO store -- without re-running HAL_TIM_PWM_ConfigChannel's full
 *          CCMR/CCER/CCR rewrite. Safe to call from any context, including ISR.
 *          The channel must already have been set up via ConfigureChannel().
 */
bool PWM::SetDutyCycle(Channel channel, float dutyCycle)
{
    if (!mInitialized) { return false; }

    const uint32_t pulse = CalculatePulse(dutyCycle, mHandle.Init.Period);
    __HAL_TIM_SET_COMPARE(&mHandle, GetChannel(channel), pulse);
    return true;
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
 * \brief   Set the PWM timer instance into internal administration.
 * \param   instance    The PWM timer instance to use.
 * \note    Asserts if the PWM timer instance is invalid.
 */
void PWM::SetInstance(const PwmTimerInstance& instance)
{
    switch (instance)
    {
        case PwmTimerInstance::TIMER_2: mHandle.Instance = TIM2; break;
        case PwmTimerInstance::TIMER_3: mHandle.Instance = TIM3; break;
        case PwmTimerInstance::TIMER_4: mHandle.Instance = TIM4; break;
        case PwmTimerInstance::TIMER_5: mHandle.Instance = TIM5; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Enable the peripheral clock for the PWM timer instance.
 * \param   instance    The PWM timer instance to enable the clock for.
 * \note    TIM2/3/4/5 sit on APB1; the underlying CLK_ENABLE macros are
 *          idempotent so no IS_CLK_DISABLED guard is needed.
 */
void PWM::CheckAndEnablePeripheralClock(const PwmTimerInstance& instance)
{
    switch (instance)
    {
        case PwmTimerInstance::TIMER_2: __HAL_RCC_TIM2_CLK_ENABLE(); break;
        case PwmTimerInstance::TIMER_3: __HAL_RCC_TIM3_CLK_ENABLE(); break;
        case PwmTimerInstance::TIMER_4: __HAL_RCC_TIM4_CLK_ENABLE(); break;
        case PwmTimerInstance::TIMER_5: __HAL_RCC_TIM5_CLK_ENABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Disable the peripheral clock for the PWM timer instance.
 * \param   instance    The PWM timer instance to disable the clock for.
 */
void PWM::CheckAndDisablePeripheralClock(const PwmTimerInstance& instance)
{
    switch (instance)
    {
        case PwmTimerInstance::TIMER_2: __HAL_RCC_TIM2_CLK_DISABLE(); break;
        case PwmTimerInstance::TIMER_3: __HAL_RCC_TIM3_CLK_DISABLE(); break;
        case PwmTimerInstance::TIMER_4: __HAL_RCC_TIM4_CLK_DISABLE(); break;
        case PwmTimerInstance::TIMER_5: __HAL_RCC_TIM5_CLK_DISABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Return the input clock feeding the PWM timer instance.
 * \details TIM2/3/4/5 are clocked from APB1. Per RM0090 §6.2 the timer
 *          input clock equals APB1 when the APB1 prescaler is 1, otherwise
 *          twice APB1.
 * \returns Timer input clock in Hz.
 */
uint32_t PWM::GetTimerInputClockFreq()
{
    const uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
    return ((RCC->CFGR & RCC_CFGR_PPRE1) >= RCC_CFGR_PPRE1_DIV2) ? (apb1 * 2U) : apb1;
}

/**
 * \brief   Calculate the PWM period value.
 * \param   desiredFrequency    The desired frequency in Hz to use.
 * \returns Period value (TIMx_ARR).
 * \note    Reads the actual timer input clock so the period is correct
 *          regardless of board clock tree.
 */
uint16_t PWM::CalculatePeriod(float desiredFrequency)
{
    EXPECT(desiredFrequency > 0.1f);

    const uint32_t tick = GetTimerInputClockFreq();

    // timer_period = (timer_tick_frequency / PWM_frequency) - 1

    uint32_t timer_period = (tick / desiredFrequency) - 1;

    // Cap at UINT16_MAX. TIM2/5 ARR is 32-bit but TIM3/4 is 16-bit; use the
    // common cap so the same Config range applies on every supported instance.
    if ((timer_period == 0) || (timer_period > UINT16_MAX))
    {
        timer_period = UINT16_MAX;
        EXPECT(false);
    }

    return static_cast<uint16_t>(timer_period);
}

/**
 * \brief   Calculate the PWM pulse value (duty cycle).
 * \param   desiredDutyCycle    The desired duty cycle, as fraction [0.0 .. 1.0].
 * \param   period              The configured period of the PWM (frequency).
 * \returns Pulse value (TIMx_CCRx).
 */
uint32_t PWM::CalculatePulse(float desiredDutyCycle, uint32_t period)
{
    EXPECT(desiredDutyCycle >= 0.0f);
    EXPECT(desiredDutyCycle <= 1.0f);

    if (desiredDutyCycle < 0.0f) { desiredDutyCycle = 0.0f; }
    if (desiredDutyCycle > 1.0f) { desiredDutyCycle = 1.0f; }

    // 0% must produce CCR=0. Underflow on `0 - 1` would wrap to 0xFFFFFFFF
    // which (truncated to the CCR register width) ends up larger than ARR;
    // CNT never reaches CCR, output stays inactive, and with the inverted
    // OCPolarity below the channel sticks at the user's "ON" level -- i.e.
    // 0% silently turns into 100%. Special-case it.
    const uint32_t scaled = static_cast<uint32_t>((period + 1U) * desiredDutyCycle);
    if (scaled == 0U) { return 0U; }

    // pulse_length = (timer_period + 1) * duty_cycle - 1
    return scaled - 1U;
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
    EXPECT(result);
    result &= Stop(Channel::Channel_2);
    EXPECT(result);
    result &= Stop(Channel::Channel_3);
    EXPECT(result);
    result &= Stop(Channel::Channel_4);
    EXPECT(result);

    return result;
}
