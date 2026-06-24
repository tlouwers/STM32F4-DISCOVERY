/**
 * \file    GenericTimer.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class to provide general elapsed timer functionality.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/GenericTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/GenericTimer/GenericTimer.hpp"
#include "utility/Assert/Assert.h"
#include "utility/ScopedIrqMask/ScopedIrqMask.hpp"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static GenericTimerCallbacks sTimer2Callback {};
static GenericTimerCallbacks sTimer3Callback {};
static GenericTimerCallbacks sTimer4Callback {};
static GenericTimerCallbacks sTimer5Callback {};
static GenericTimerCallbacks sTimer9Callback {};
static GenericTimerCallbacks sTimer10Callback {};
static GenericTimerCallbacks sTimer11Callback {};
static GenericTimerCallbacks sTimer12Callback {};
static GenericTimerCallbacks sTimer13Callback {};
static GenericTimerCallbacks sTimer14Callback {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackElapsed, if configured.
 * \param   timer_callback  Structure containing the callbackElapsed to call.
 */
static void CallbackElapsed(const GenericTimerCallbacks& timer_callback)
{
    if (timer_callback.callbackElapsed)
    {
        timer_callback.callbackElapsed();
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal GenericTimer instance administration.
 * \param   instance    The GenericTimer instance to use.
 */
GenericTimer::GenericTimer(const GenericTimerInstance& instance) :
    mInstance(instance),
    mGenericTimerCallback( ((instance == GenericTimerInstance::TIMER_2) ? sTimer2Callback :
                           ((instance == GenericTimerInstance::TIMER_3) ? sTimer3Callback :
                           ((instance == GenericTimerInstance::TIMER_4) ? sTimer4Callback :
                           ((instance == GenericTimerInstance::TIMER_5) ? sTimer5Callback :
                           ((instance == GenericTimerInstance::TIMER_9) ? sTimer9Callback :
                           ((instance == GenericTimerInstance::TIMER_10) ? sTimer10Callback :
                           ((instance == GenericTimerInstance::TIMER_11) ? sTimer11Callback :
                           ((instance == GenericTimerInstance::TIMER_12) ? sTimer12Callback :
                           ((instance == GenericTimerInstance::TIMER_13) ? sTimer13Callback : sTimer14Callback))))))))) ),
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
GenericTimer::~GenericTimer()
{
    Sleep();
    DisconnectCallbacks();
}

/**
 * \brief   Initializes the GenericTimer instance with the given configuration.
 * \param   config  The configuration for the GenericTimer instance to use.
 * \returns True if the configuration could be applied, else false.
 * \note    The prescaler is computed from the actual APB1/APB2 timer clock
 *          (depending on the instance) so that CK_CNT lands at 10 kHz
 *          regardless of board clock tree.
 */
bool GenericTimer::Init(const IConfig& config)
{
    CheckAndEnablePeripheralClock(mInstance);

    EXPECT(config.ConfigId() == Config::Id());
    if (config.ConfigId() != Config::Id()) { return false; }

    const Config& cfg = static_cast<const Config&>(config);

    EXPECT(cfg.mFrequency > 0.0f);
    if (cfg.mFrequency <= 0.0f) { return false; }

    // CK_CNT is 10 kHz, so 10000 Hz (ARR = 0) is the highest representable rate.
    EXPECT(cfg.mFrequency <= 10000.0f);
    if (cfg.mFrequency > 10000.0f) { return false; }

    // (Freq. timer input) / (Prescaler + 1) = (Freq. CK_CNT) --> aim for 10 kHz CNT
    mHandle.Init.Prescaler         = (GetTimerInputClockFreq(mInstance) / 10000U) - 1U;
    mHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    mHandle.Init.Period            = CalculatePeriod(cfg.mFrequency);    // (Freq. desired) = (Freq. CK_CNT) / (TIM_ARR + 1)
    mHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    mHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&mHandle) == HAL_OK)
    {
        // Own this timer's vector through the shared TimerIRQ dispatcher BEFORE
        // the NVIC line goes live: TimerIRQ requires the slot be armed while the
        // line is disabled (std::function assignment is not atomic on Cortex-M4).
        // SetIRQn enables the line as its final step.
        TimerIRQ::Install(GetSlot(mInstance), [this]() { HAL_TIM_IRQHandler(&mHandle); });

        // Configure NVIC to generate interrupt
        SetIRQn(GetIRQn(mInstance), cfg.mInterruptPriority, 0);

        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if GenericTimer is initialized.
 * \returns True if GenericTimer is initialized, else false.
 */
bool GenericTimer::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the GenericTimer module in sleep mode.
 * \details Stops the timer.
 * \returns True if timer could be put in sleep mode, else false.
 */
bool GenericTimer::Sleep()
{
    Stop();
    mStarted = false;   // force-clear even if Stop()'s HAL stop was rejected

    // Tear the vector down before de-init: disable the NVIC line and drop the
    // dispatcher slot first, so no stale lambda capturing `this` can be reached
    // during or after the teardown, then de-init the peripheral.
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance) );
    DisconnectCallbacks();

    if (HAL_TIM_Base_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    CheckAndDisablePeripheralClock(mInstance);
    return true;
}

/**
 * \brief   Starts the timer.
 * \param   handler     Callback to call when timer elapsed.
 * \returns True if the timer could be started, else false.
 */
bool GenericTimer::Start(const std::function<void()>& handler)
{
    if (!mInitialized) { return false; }

    if (!mStarted)
    {
        // Arm the handler only after the HAL accepts the start, bracketed by the
        // per-instance NVIC mask (ScopedIrqMask): the std::function assignment is
        // not atomic and, once the IRQ is live, the timer ISR may read the slot.
        // A rejected start (HAL_BUSY/HAL_ERROR) leaves no stale handler armed.
        ScopedIrqMask mask(GetIRQn(mInstance));
        if (HAL_TIM_Base_Start_IT(&mHandle) != HAL_OK) { return false; }
        mGenericTimerCallback.callbackElapsed = handler;
        mStarted = true;
    }

    return true;
}

/**
 * \brief   Indicate if GenericTimer is started.
 * \returns True if GenericTimer is started, else false.
 */
bool GenericTimer::IsStarted() const
{
    return mStarted;
}

/**
 * \brief   Stops the timer.
 * \returns True if the timer could be stopped, else false.
 */
bool GenericTimer::Stop()
{
    if (!mInitialized) { return false; }

    if (mStarted)
    {
        if (HAL_TIM_Base_Stop_IT(&mHandle) != HAL_OK) { return false; }
        mStarted = false;
    }

    return true;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the GenericTimer instance into internal administration.
 * \param   instance    The GenericTimer instance to use.
 * \note    Asserts if the GenericTimer instance is invalid.
 */
void GenericTimer::SetInstance(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  mHandle.Instance = TIM2;  break;
        case GenericTimerInstance::TIMER_3:  mHandle.Instance = TIM3;  break;
        case GenericTimerInstance::TIMER_4:  mHandle.Instance = TIM4;  break;
        case GenericTimerInstance::TIMER_5:  mHandle.Instance = TIM5;  break;
        case GenericTimerInstance::TIMER_9:  mHandle.Instance = TIM9;  break;
        case GenericTimerInstance::TIMER_10: mHandle.Instance = TIM10; break;
        case GenericTimerInstance::TIMER_11: mHandle.Instance = TIM11; break;
        case GenericTimerInstance::TIMER_12: mHandle.Instance = TIM12; break;
        case GenericTimerInstance::TIMER_13: mHandle.Instance = TIM13; break;
        case GenericTimerInstance::TIMER_14: mHandle.Instance = TIM14; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Enable the peripheral clock for the GenericTimer instance.
 * \param   instance    The GenericTimer instance to enable the clock for.
 * \note    TIM2/3/4/5/12/13/14 sit on APB1, TIM9/10/11 on APB2; the
 *          underlying CLK_ENABLE macros are idempotent so no IS_CLK_DISABLED
 *          guard is needed.
 */
void GenericTimer::CheckAndEnablePeripheralClock(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  __HAL_RCC_TIM2_CLK_ENABLE();  break;
        case GenericTimerInstance::TIMER_3:  __HAL_RCC_TIM3_CLK_ENABLE();  break;
        case GenericTimerInstance::TIMER_4:  __HAL_RCC_TIM4_CLK_ENABLE();  break;
        case GenericTimerInstance::TIMER_5:  __HAL_RCC_TIM5_CLK_ENABLE();  break;
        case GenericTimerInstance::TIMER_9:  __HAL_RCC_TIM9_CLK_ENABLE();  break;
        case GenericTimerInstance::TIMER_10: __HAL_RCC_TIM10_CLK_ENABLE(); break;
        case GenericTimerInstance::TIMER_11: __HAL_RCC_TIM11_CLK_ENABLE(); break;
        case GenericTimerInstance::TIMER_12: __HAL_RCC_TIM12_CLK_ENABLE(); break;
        case GenericTimerInstance::TIMER_13: __HAL_RCC_TIM13_CLK_ENABLE(); break;
        case GenericTimerInstance::TIMER_14: __HAL_RCC_TIM14_CLK_ENABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Disable the peripheral clock for the GenericTimer instance.
 * \param   instance    The GenericTimer instance to disable the clock for.
 */
void GenericTimer::CheckAndDisablePeripheralClock(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  __HAL_RCC_TIM2_CLK_DISABLE();  break;
        case GenericTimerInstance::TIMER_3:  __HAL_RCC_TIM3_CLK_DISABLE();  break;
        case GenericTimerInstance::TIMER_4:  __HAL_RCC_TIM4_CLK_DISABLE();  break;
        case GenericTimerInstance::TIMER_5:  __HAL_RCC_TIM5_CLK_DISABLE();  break;
        case GenericTimerInstance::TIMER_9:  __HAL_RCC_TIM9_CLK_DISABLE();  break;
        case GenericTimerInstance::TIMER_10: __HAL_RCC_TIM10_CLK_DISABLE(); break;
        case GenericTimerInstance::TIMER_11: __HAL_RCC_TIM11_CLK_DISABLE(); break;
        case GenericTimerInstance::TIMER_12: __HAL_RCC_TIM12_CLK_DISABLE(); break;
        case GenericTimerInstance::TIMER_13: __HAL_RCC_TIM13_CLK_DISABLE(); break;
        case GenericTimerInstance::TIMER_14: __HAL_RCC_TIM14_CLK_DISABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Return the input clock feeding the given GenericTimer instance.
 * \details TIM2/3/4/5/12/13/14 are clocked from APB1; TIM9/10/11 from APB2.
 *          The timer input clock depends on the APB prescaler AND the
 *          RCC_DCKCFGR.TIMPRE bit (RM0090 §6.2):
 *            - TIMPRE = 0 (reset default): TIMxCLK = PCLKx when the APB
 *              prescaler is 1, otherwise 2 x PCLKx.
 *            - TIMPRE = 1: TIMxCLK = HCLK when the APB prescaler is 1, 2 or 4,
 *              otherwise 4 x PCLKx.
 * \param   instance    The GenericTimer instance to query.
 * \returns Timer input clock in Hz.
 */
uint32_t GenericTimer::GetTimerInputClockFreq(const GenericTimerInstance& instance)
{
    // Select the APB bus feeding this timer (RM0090 §6.2 / F407 datasheet Table 52).
    const bool isApb2 = (instance == GenericTimerInstance::TIMER_9)  ||
                        (instance == GenericTimerInstance::TIMER_10) ||
                        (instance == GenericTimerInstance::TIMER_11);

    const uint32_t pclk    = isApb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
    const uint32_t ppreMsk = isApb2 ? RCC_CFGR_PPRE2      : RCC_CFGR_PPRE1;
    const uint32_t ppreVal =  RCC->CFGR & ppreMsk;
    const uint32_t div2    = isApb2 ? RCC_CFGR_PPRE2_DIV2 : RCC_CFGR_PPRE1_DIV2;
    const uint32_t div4    = isApb2 ? RCC_CFGR_PPRE2_DIV4 : RCC_CFGR_PPRE1_DIV4;

    if ((RCC->DCKCFGR & RCC_DCKCFGR_TIMPRE) == 0U)
    {
        // TIMPRE = 0: x1 when APB prescaler is 1, otherwise x2.
        return (ppreVal >= div2) ? (pclk * 2U) : pclk;
    }

    // TIMPRE = 1: HCLK when APB prescaler is 1, 2 or 4, otherwise 4 x PCLK.
    return (ppreVal <= div4) ? HAL_RCC_GetHCLKFreq() : (pclk * 4U);
}

/**
 * \brief   Calculate the GenericTimer period value.
 * \param   desiredFrequency    The desired frequency in Hz to use.
 * \returns Period value (TIM_ARR).
 * \note    The CK_CNT is assumed to be 10 kHz.
 * \note    TIM2 and TIM5 are 32-bit on the F4 (RM0090 §17.3.1), so their
 *          ARR may use the full 0xFFFFFFFF range -- allowing far lower
 *          frequencies than the ~0.153 Hz floor a 16-bit cap imposes.
 *          Every other GenericTimer instance is 16-bit and stays clamped
 *          to 0xFFFF (unchanged behaviour).
 */
uint32_t GenericTimer::CalculatePeriod(float desiredFrequency)
{
    // Freq. CK_CNT is 10 kHz
    // (Freq. desired) = (Freq. CK_CNT) / (TIM_ARR + 1)
    // (10000 / desiredFrequency) - 1 = TIM_ARR

    const bool is32Bit = (mInstance == GenericTimerInstance::TIMER_2) ||
                         (mInstance == GenericTimerInstance::TIMER_5);
    const uint32_t maxPeriod = is32Bit ? 0xFFFFFFFFUL : static_cast<uint32_t>(UINT16_MAX);

    // Clamp in the float domain BEFORE the cast: a very small (but positive)
    // desiredFrequency makes the intermediate exceed UINT32_MAX, and casting an
    // out-of-range float to uint32_t is undefined behaviour on Cortex-M.
    const float rawPeriod = (10000.0f / desiredFrequency) - 1.0f;
    if (rawPeriod >= static_cast<float>(maxPeriod)) { return maxPeriod; }
    if (rawPeriod <= 0.0f)                          { return 0U; }

    return static_cast<uint32_t>(rawPeriod);
}

/**
 * \brief   Get the IRQ belonging to the GenericTimer.
 * \param   instance    The GenericTimer instance to get the IRQ for.
 * \returns The interrupt line IRQ to which the GenericTimer belongs. If invalid
 *          instance provided this function will hang has no proper IRQ can be
 *          found.
 * \note    Asserts if not a valid GenericTimer instance provided.
 */
IRQn_Type GenericTimer::GetIRQn(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  return TIM2_IRQn;               break;
        case GenericTimerInstance::TIMER_3:  return TIM3_IRQn;               break;
        case GenericTimerInstance::TIMER_4:  return TIM4_IRQn;               break;
        case GenericTimerInstance::TIMER_5:  return TIM5_IRQn;               break;
        case GenericTimerInstance::TIMER_9:  return TIM1_BRK_TIM9_IRQn;      break;
        case GenericTimerInstance::TIMER_10: return TIM1_UP_TIM10_IRQn;      break;
        case GenericTimerInstance::TIMER_11: return TIM1_TRG_COM_TIM11_IRQn; break;
        case GenericTimerInstance::TIMER_12: return TIM8_BRK_TIM12_IRQn;     break;
        case GenericTimerInstance::TIMER_13: return TIM8_UP_TIM13_IRQn;      break;
        case GenericTimerInstance::TIMER_14: return TIM8_TRG_COM_TIM14_IRQn; break;
        default: ASSERT(false); while(1) { __NOP(); } return TIM2_IRQn;      break;      // Impossible selection
    }
}

/**
 * \brief   Lower level configuration for the GenericTimer interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void GenericTimer::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
}

/**
 * \brief   Get the TimerIRQ dispatcher slot belonging to the GenericTimer.
 * \param   instance    The GenericTimer instance to get the slot for.
 * \returns The TimerIRQ slot to which the GenericTimer belongs.
 * \note    Asserts if not a valid GenericTimer instance provided.
 */
TimerIRQ::Slot GenericTimer::GetSlot(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  return TimerIRQ::Slot::TIMER_2;  break;
        case GenericTimerInstance::TIMER_3:  return TimerIRQ::Slot::TIMER_3;  break;
        case GenericTimerInstance::TIMER_4:  return TimerIRQ::Slot::TIMER_4;  break;
        case GenericTimerInstance::TIMER_5:  return TimerIRQ::Slot::TIMER_5;  break;
        case GenericTimerInstance::TIMER_9:  return TimerIRQ::Slot::TIMER_9;  break;
        case GenericTimerInstance::TIMER_10: return TimerIRQ::Slot::TIMER_10; break;
        case GenericTimerInstance::TIMER_11: return TimerIRQ::Slot::TIMER_11; break;
        case GenericTimerInstance::TIMER_12: return TimerIRQ::Slot::TIMER_12; break;
        case GenericTimerInstance::TIMER_13: return TimerIRQ::Slot::TIMER_13; break;
        case GenericTimerInstance::TIMER_14: return TimerIRQ::Slot::TIMER_14; break;
        default: ASSERT(false); while(1) { __NOP(); } return TimerIRQ::Slot::TIMER_2; break;      // Impossible selection
    }
}

/**
 * \brief   Drop the elapsed callback and release the TimerIRQ slot for this
 *          GenericTimer instance.
 * \note    Called from Sleep() and as a destructor safety net so no stale
 *          lambda capturing `this` outlives the object on the shared
 *          TimerIRQ dispatcher slot.
 */
void GenericTimer::DisconnectCallbacks()
{
    TimerIRQ::Uninstall(GetSlot(mInstance));
    mGenericTimerCallback.callbackElapsed = nullptr;
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the timer elapsed interrupt into an
 *          Elapsed callback.
 * \param   handle  The GenericTimer handle from which the timer elapsed ISR
 *          came.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* handle)
{
    ASSERT(handle);

    // handle->Instance is a unique peripheral pointer, so at most one arm
    // matches: else-if avoids the remaining comparisons in ISR context.
    if      (handle->Instance == TIM2)  { CallbackElapsed(sTimer2Callback);  }
    else if (handle->Instance == TIM3)  { CallbackElapsed(sTimer3Callback);  }
    else if (handle->Instance == TIM4)  { CallbackElapsed(sTimer4Callback);  }
    else if (handle->Instance == TIM5)  { CallbackElapsed(sTimer5Callback);  }
    else if (handle->Instance == TIM9)  { CallbackElapsed(sTimer9Callback);  }
    else if (handle->Instance == TIM10) { CallbackElapsed(sTimer10Callback); }
    else if (handle->Instance == TIM11) { CallbackElapsed(sTimer11Callback); }
    else if (handle->Instance == TIM12) { CallbackElapsed(sTimer12Callback); }
    else if (handle->Instance == TIM13) { CallbackElapsed(sTimer13Callback); }
    else if (handle->Instance == TIM14) { CallbackElapsed(sTimer14Callback); }
}
