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
 * \brief   BasicTimer class used for the DAC to drive the DMA based output sampling.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/BasicTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/BasicTimer/BasicTimer.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal BasicTimer instance administration.
 * \param   instance    The BasicTimer instance to use.
 */
BasicTimer::BasicTimer(const BasicTimerInstance& instance) :
    mInstance(instance),
    mInitialized(false),
    mStarted(false)
{
    SetInstance(instance);
}

/**
 * \brief   Destructor, stops the timer and disables interrupts.
 * \note    DisconnectCallbacks is also invoked unconditionally here as a
 *          safety net so a stale lambda capturing this object's `this`
 *          cannot be dispatched after destruction.
 */
BasicTimer::~BasicTimer()
{
    Sleep();
    DisconnectCallbacks();
}

/**
 * \brief   Initializes the BasicTimer instance with the given configuration.
 * \param   config  The configuration for the BasicTimer instance to use.
 * \returns True if the configuration could be applied, else false.
 * \note    The prescaler is computed from the actual APB1 timer clock so
 *          that CNT_CLK lands at 1 MHz regardless of board clock tree.
 */
bool BasicTimer::Init(const IConfig& config)
{
    CheckAndEnablePeripheralClock(mInstance);

    const Config& cfg = reinterpret_cast<const Config&>(config);

    EXPECT(cfg.mFrequency > 0);
    if (cfg.mFrequency == 0) { return false; }

    // (Freq. timer input) / (Prescaler + 1) = (Freq. CLK_CNT) --> aim for 1 MHz CNT
    mHandle.Init.Prescaler         = (GetTimerInputClockFreq() / 1000000U) - 1U;
    mHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    mHandle.Init.Period            = CalculatePeriod(cfg.mFrequency); // (Freq. desired) = (Freq. CNT_CLK) / (TIM_ARR + 1)
    mHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    mHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&mHandle) == HAL_OK)
    {
        TIM_MasterConfigTypeDef masterConfig = {};
        masterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
        masterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&mHandle, &masterConfig) == HAL_OK)
        {
            // Configure NVIC to generate interrupt
            SetIRQn(GetIRQn(mInstance), cfg.mInterruptPriority, 0);

            // Own this timer's vector through the shared TimerIRQ dispatcher.
            // Note: Start() uses HAL_TIM_Base_Start (no UDIE), so this slot is
            // wired but currently never fires -- see BasicTimer IRQ deferral.
            TimerIRQ::Install(GetSlot(mInstance), [this]() { HAL_TIM_IRQHandler(&mHandle); });

            mInitialized = true;
            return true;
        }
    }
    return false;
}

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
 * \returns True if timer could be put in sleep mode, else false.
 */
bool BasicTimer::Sleep()
{
    Stop();

    if (HAL_TIM_Base_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    HAL_NVIC_DisableIRQ( GetIRQn(mInstance) );

    DisconnectCallbacks();

    CheckAndDisablePeripheralClock(mInstance);
    return true;
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
 * \brief   Enable the peripheral clock for the BasicTimer instance.
 * \param   instance    The BasicTimer instance to enable the clock for.
 * \note    TIM6 and TIM7 sit on APB1; the underlying CLK_ENABLE macros
 *          are idempotent so no IS_CLK_DISABLED guard is needed.
 */
void BasicTimer::CheckAndEnablePeripheralClock(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: __HAL_RCC_TIM6_CLK_ENABLE(); break;
        case BasicTimerInstance::TIMER_7: __HAL_RCC_TIM7_CLK_ENABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Disable the peripheral clock for the BasicTimer instance.
 * \param   instance    The BasicTimer instance to disable the clock for.
 */
void BasicTimer::CheckAndDisablePeripheralClock(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: __HAL_RCC_TIM6_CLK_DISABLE(); break;
        case BasicTimerInstance::TIMER_7: __HAL_RCC_TIM7_CLK_DISABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Return the input clock feeding TIM6/TIM7.
 * \details TIM6 and TIM7 are clocked from APB1. Per RM0090 §6.2 the timer
 *          input clock equals APB1 when the APB1 prescaler is 1, otherwise
 *          twice APB1.
 * \returns Timer input clock in Hz.
 */
uint32_t BasicTimer::GetTimerInputClockFreq()
{
    const uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
    return ((RCC->CFGR & RCC_CFGR_PPRE1) >= RCC_CFGR_PPRE1_DIV2) ? (apb1 * 2U) : apb1;
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
 * \brief   Get the TimerIRQ dispatcher slot belonging to the BasicTimer.
 * \param   instance    The BasicTimer instance to get the slot for.
 * \returns The TimerIRQ slot to which the BasicTimer belongs.
 * \note    Asserts if not a valid BasicTimer instance provided.
 */
TimerIRQ::Slot BasicTimer::GetSlot(const BasicTimerInstance& instance)
{
    switch (instance)
    {
        case BasicTimerInstance::TIMER_6: return TimerIRQ::Slot::TIMER_6; break;
        case BasicTimerInstance::TIMER_7: return TimerIRQ::Slot::TIMER_7; break;
        default: ASSERT(false); while(1) { __NOP(); } return TimerIRQ::Slot::TIMER_6; break;      // Impossible selection
    }
}

/**
 * \brief   Release the TimerIRQ slot for this BasicTimer instance.
 * \note    Called from Sleep() and as a destructor safety net so no stale
 *          lambda capturing `this` outlives the object on the shared
 *          TimerIRQ dispatcher slot.
 */
void BasicTimer::DisconnectCallbacks()
{
    TimerIRQ::Uninstall(GetSlot(mInstance));
}
