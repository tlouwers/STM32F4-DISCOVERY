/**
 * \file    BasicTimer.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   BasicTimer peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/BasicTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/BasicTimer/BasicTimer.hpp"
#include "utility/SlimAssert/SlimAssert.h"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static BasicTimerCallback timer6_callback {};
static BasicTimerCallback timer7_callback {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   timer_callback  Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const BasicTimerCallback& timer_callback)
{
    if (timer_callback.callbackIRQ)
    {
        timer_callback.callbackIRQ();
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal BasicTimer instance administration.
 * \param   instance    The BasicTImer instance to use.
 */
BasicTimer::BasicTimer(const BasicTimerInstance& instance) :
    mInstance(instance),
    mBasicTimerCallback( (instance == BasicTimerInstance::TIMER_6) ? (timer6_callback) : (timer7_callback) ),
    mInitialized(false),
    mStarted(false)
{
    SetInstance(instance);

    mBasicTimerCallback.callbackIRQ = [this]() { this->CallbackIRQ(); };
}

/**
 * \brief   Destructor, stop timer, disabled interrupts.
 */
BasicTimer::~BasicTimer()
{
    Stop();

    // Disable interrupts
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance) );

    mInitialized = false;
}

/**
 * \brief   Initializes the BasicTimer instance with the given configuration.
 * \param   config  The configuration for the BasicTimer instance to use.
 * \returns True if the configuration could be applied, else false.
 * \note    APB1 assumed to be 8 MHz.
 */
bool BasicTimer::Init(const Config& config)
{
    CheckAndEnableAHB1PeripheralClock(mInstance);

    mHandle.Init.Prescaler         = 8 - 1;                              // (Freq. APB1) / (Prescaler + 1) = (Freq. CLK_CNT) --> Get from 8 MHz to 1 MHz as timer counter
    mHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    mHandle.Init.Period            = CalculatePeriod(config.mFrequency); // (Freq. desired) = (Freq. CNT_CLK) / (TIM_ARR + 1)
    mHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    mHandle.Init.RepetitionCounter = 0;
    mHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&mHandle) == HAL_OK)
    {
        TIM_MasterConfigTypeDef masterConfig = {};
        masterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
        masterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&mHandle, &masterConfig) == HAL_OK)
        {
            // Configure NVIC to generate interrupt
            SetIRQn(GetIRQn(mInstance), config.mInterruptPriority, 0);

            mInitialized = true;
            return true;
        }
    }
    return false;
};

/**
 * \brief   Indicate if BasicTimer is initialized.
 * \returns True if BasicTimer is initialized, else false.
 */
bool BasicTimer::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the BasicTimer module in sleep mode.
 * \details Stops the timer.
 */
void BasicTimer::Sleep()
{
    Stop();

    // Disable interrupts
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance) );

    mInitialized = false;

    // ToDo: low power state, check recovery after sleep
}

/**
 * \brief   Starts the timer.
 * \returns True if the timer could be started, else false.
 */
bool BasicTimer::Start()
{
    if (mInitialized)
    {
        if (! mStarted)
        {
            HAL_TIM_Base_Start(&mHandle);
            mStarted = true;
        }
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if BasicTimer is started.
 * \returns True if BasicTimer is started, else false.
 */
bool BasicTimer::IsStarted() const
{
    return mStarted;
}

/**
 * \brief   Stops the timer.
 * \returns True if the timer could be stopped, else false.
 */
bool BasicTimer::Stop()
{
    if (mInitialized)
    {
        if (mStarted)
        {
            HAL_TIM_Base_Stop(&mHandle);
            mStarted = false;
        }
        return true;
    }
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the BasicTimer instance into internal administration.
 * \param   instance    The BasicTimer instance to use.
 * \note    Asserts if the BasicTimer instance is invalid.
 */
void BasicTimer::SetInstance(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: mHandle.Instance = TIM6; break;
        case BasicTimerInstance::TIMER_7: mHandle.Instance = TIM7; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the BasicTimer
 *          instance is enabled, if not enable it.
 * \param   instance    The BasicTimer instance to enable the clock for.
 * \note    Asserts if not a valid BasicTimer instance provided.
 */
void BasicTimer::CheckAndEnableAHB1PeripheralClock(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: if (__HAL_RCC_TIM6_IS_CLK_DISABLED()) { __HAL_RCC_TIM6_CLK_ENABLE(); } break;
        case BasicTimerInstance::TIMER_7: if (__HAL_RCC_TIM7_IS_CLK_DISABLED()) { __HAL_RCC_TIM7_CLK_ENABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Calculate the BasicTimer period value.
 * \param   desiredFrequency    The desired frequency in Hz to use.
 * \returns Period value (TIM_ARR).
 * \note    The CLK_CNT is assumed to be 1 MHz.
 */
uint16_t BasicTimer::CalculatePeriod(uint16_t desiredFrequency)
{
    // Freq. CLK_CNT is 1 MHz
    // (Freq. desired) = (Freq. CNT_CLK) / (TIM_ARR + 1)
    // (1000000 / desiredFrequency) - 1 = TIM_ARR

    uint32_t period = (1000000 / desiredFrequency) - 1;

    if (period > UINT16_MAX) { period = UINT16_MAX; }

    return static_cast<uint16_t>(period);
}

/**
 * \brief   Get the IRQ belonging to the BasicTimer.
 * \param   instance    The BasicTimer instance to get the IRQ for.
 * \returns The interrupt line IRQ to which the BasicTimer belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid BasicTimer instance provided.
 */
IRQn_Type BasicTimer::GetIRQn(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: return TIM6_DAC_IRQn; break;
        case BasicTimerInstance::TIMER_7: return TIM7_IRQn; break;
        default: ASSERT(false); while(1) { __NOP(); } return TIM6_DAC_IRQn; break;      // Impossible selection
    }
}

/**
 * \brief   Lower level configuration for the BasicTimer interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void BasicTimer::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
}

/**
 * \brief   Generic BasicTimer IRQ callback. Will propagate other interrupts.
 */
void BasicTimer::CallbackIRQ()
{
    HAL_TIM_IRQHandler(&mHandle);
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: route TIM6 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM6_IRQHandler(void)
{
    CallbackIRQ(timer6_callback);
}

/**
 * \brief   ISR: route TIM7 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM7_IRQHandler(void)
{
    CallbackIRQ(timer7_callback);
}
