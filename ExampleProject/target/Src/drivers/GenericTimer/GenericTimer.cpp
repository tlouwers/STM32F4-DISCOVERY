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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/GenericTimer
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
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static GenericTimerCallbacks timer2_callback {};
static GenericTimerCallbacks timer3_callback {};
static GenericTimerCallbacks timer4_callback {};
static GenericTimerCallbacks timer5_callback {};
static GenericTimerCallbacks timer9_callback {};
static GenericTimerCallbacks timer10_callback {};
static GenericTimerCallbacks timer11_callback {};
static GenericTimerCallbacks timer12_callback {};
static GenericTimerCallbacks timer13_callback {};
static GenericTimerCallbacks timer14_callback {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   timer_callback  Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const GenericTimerCallbacks& timer_callback)
{
    if (timer_callback.callbackIRQ)
    {
        timer_callback.callbackIRQ();
    }
}

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
    mGenericTimerCallback( ((instance == GenericTimerInstance::TIMER_2) ? timer2_callback :
                           ((instance == GenericTimerInstance::TIMER_3) ? timer3_callback :
                           ((instance == GenericTimerInstance::TIMER_4) ? timer4_callback :
                           ((instance == GenericTimerInstance::TIMER_5) ? timer5_callback :
                           ((instance == GenericTimerInstance::TIMER_9) ? timer9_callback :
                           ((instance == GenericTimerInstance::TIMER_10) ? timer10_callback :
                           ((instance == GenericTimerInstance::TIMER_11) ? timer11_callback :
                           ((instance == GenericTimerInstance::TIMER_12) ? timer12_callback :
                           ((instance == GenericTimerInstance::TIMER_13) ? timer13_callback : timer14_callback))))))))) ),
    mInitialized(false),
    mStarted(false)
{
    SetInstance(instance);

    mGenericTimerCallback.callbackIRQ = [this]() { this->CallbackIRQ(); };
}

/**
 * \brief   Destructor, stop timer, disabled interrupts.
 */
GenericTimer::~GenericTimer()
{
    Sleep();
}

/**
 * \brief   Initializes the GenericTimer instance with the given configuration.
 * \param   config  The configuration for the GenericTimer instance to use.
 * \returns True if the configuration could be applied, else false.
 * \note    APB1 and APB2 assumed to be 8 MHz.
 */
bool GenericTimer::Init(const IConfig& config)
{
    CheckAndEnableAHBPeripheralClock(mInstance);

    const Config& cfg = reinterpret_cast<const Config&>(config);

    mHandle.Init.Prescaler         = 800 - 1;                            // (Freq. APB) / (Prescaler + 1) = (Freq. CK_CNT) --> Get from 8 MHz to 10 kHz as timer counter
    mHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    mHandle.Init.Period            = CalculatePeriod(cfg.mFrequency);    // (Freq. desired) = (Freq. CK_CNT) / (TIM_ARR + 1)
    mHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    mHandle.Init.RepetitionCounter = 0;
    mHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&mHandle) == HAL_OK)
    {
        // Configure NVIC to generate interrupt
        SetIRQn(GetIRQn(mInstance), cfg.mInterruptPriority, 0);

        mInitialized = true;
        return true;
    }
    return false;
};

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

    mInitialized = false;

    // Disable interrupts
    HAL_NVIC_DisableIRQ( GetIRQn(mInstance) );

    if (HAL_TIM_Base_DeInit(&mHandle) == HAL_OK)
    {
        CheckAndDisableAHBPeripheralClock(mInstance);
        return true;
    }
    return false;
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
        mGenericTimerCallback.callbackElapsed = handler;

        HAL_TIM_Base_Start_IT(&mHandle);
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
        HAL_TIM_Base_Stop_IT(&mHandle);
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
 * \brief   Check if the appropriate AHB1 or AHB2 peripheral clock for the GenericTimer
 *          instance is enabled, if not enable it.
 * \param   instance    The GenericTimer instance to enable the clock for.
 * \note    Asserts if not a valid GenericTimer instance provided.
 */
void GenericTimer::CheckAndEnableAHBPeripheralClock(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  if (__HAL_RCC_TIM2_IS_CLK_DISABLED())  { __HAL_RCC_TIM2_CLK_ENABLE();  } break;
        case GenericTimerInstance::TIMER_3:  if (__HAL_RCC_TIM3_IS_CLK_DISABLED())  { __HAL_RCC_TIM3_CLK_ENABLE();  } break;
        case GenericTimerInstance::TIMER_4:  if (__HAL_RCC_TIM4_IS_CLK_DISABLED())  { __HAL_RCC_TIM4_CLK_ENABLE();  } break;
        case GenericTimerInstance::TIMER_5:  if (__HAL_RCC_TIM5_IS_CLK_DISABLED())  { __HAL_RCC_TIM5_CLK_ENABLE();  } break;
        case GenericTimerInstance::TIMER_9:  if (__HAL_RCC_TIM9_IS_CLK_DISABLED())  { __HAL_RCC_TIM9_CLK_ENABLE();  } break;
        case GenericTimerInstance::TIMER_10: if (__HAL_RCC_TIM10_IS_CLK_DISABLED()) { __HAL_RCC_TIM10_CLK_ENABLE(); } break;
        case GenericTimerInstance::TIMER_11: if (__HAL_RCC_TIM11_IS_CLK_DISABLED()) { __HAL_RCC_TIM11_CLK_ENABLE(); } break;
        case GenericTimerInstance::TIMER_12: if (__HAL_RCC_TIM12_IS_CLK_DISABLED()) { __HAL_RCC_TIM12_CLK_ENABLE(); } break;
        case GenericTimerInstance::TIMER_13: if (__HAL_RCC_TIM13_IS_CLK_DISABLED()) { __HAL_RCC_TIM13_CLK_ENABLE(); } break;
        case GenericTimerInstance::TIMER_14: if (__HAL_RCC_TIM14_IS_CLK_DISABLED()) { __HAL_RCC_TIM14_CLK_ENABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB1 or AHB2 peripheral clock for the GenericTimer
 *          instance is enabled, if so disable it.
 * \param   instance    The GenericTimer instance to disable the clock for.
 * \note    Asserts if not a valid GenericTimer instance provided.
 */
void GenericTimer::CheckAndDisableAHBPeripheralClock(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  if (__HAL_RCC_TIM2_IS_CLK_ENABLED())  { __HAL_RCC_TIM2_CLK_DISABLE();  } break;
        case GenericTimerInstance::TIMER_3:  if (__HAL_RCC_TIM3_IS_CLK_ENABLED())  { __HAL_RCC_TIM3_CLK_DISABLE();  } break;
        case GenericTimerInstance::TIMER_4:  if (__HAL_RCC_TIM4_IS_CLK_ENABLED())  { __HAL_RCC_TIM4_CLK_DISABLE();  } break;
        case GenericTimerInstance::TIMER_5:  if (__HAL_RCC_TIM5_IS_CLK_ENABLED())  { __HAL_RCC_TIM5_CLK_DISABLE();  } break;
        case GenericTimerInstance::TIMER_9:  if (__HAL_RCC_TIM9_IS_CLK_ENABLED())  { __HAL_RCC_TIM9_CLK_DISABLE();  } break;
        case GenericTimerInstance::TIMER_10: if (__HAL_RCC_TIM10_IS_CLK_ENABLED()) { __HAL_RCC_TIM10_CLK_DISABLE(); } break;
        case GenericTimerInstance::TIMER_11: if (__HAL_RCC_TIM11_IS_CLK_ENABLED()) { __HAL_RCC_TIM11_CLK_DISABLE(); } break;
        case GenericTimerInstance::TIMER_12: if (__HAL_RCC_TIM12_IS_CLK_ENABLED()) { __HAL_RCC_TIM12_CLK_DISABLE(); } break;
        case GenericTimerInstance::TIMER_13: if (__HAL_RCC_TIM13_IS_CLK_ENABLED()) { __HAL_RCC_TIM13_CLK_DISABLE(); } break;
        case GenericTimerInstance::TIMER_14: if (__HAL_RCC_TIM14_IS_CLK_ENABLED()) { __HAL_RCC_TIM14_CLK_DISABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Calculate the GenericTimer period value.
 * \param   desiredFrequency    The desired frequency in Hz to use.
 * \returns Period value (TIM_ARR).
 * \note    The CK_CNT is assumed to be 10 kHz.
 *          Asserts if desiredFrequency not within valid range.
 */
uint16_t GenericTimer::CalculatePeriod(float desiredFrequency)
{
    // Freq. CK_CNT is 10 kHz
    // (Freq. desired) = (Freq. CK_CNT) / (TIM_ARR + 1)
    // (10000 / desiredFrequency) - 1 = TIM_ARR

    ASSERT(desiredFrequency > 0.0);
    ASSERT(desiredFrequency <= 10000.0);

    uint32_t period = (10000 / desiredFrequency) - 1;

    if (period > 10000) { period = 10000; }

    return static_cast<uint16_t>(period);
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
 * \brief   Generic GenericTimer IRQ callback. Will propagate other interrupts.
 */
void GenericTimer::CallbackIRQ()
{
    HAL_TIM_IRQHandler(&mHandle);
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

    if (handle->Instance == TIM2)  { CallbackElapsed(timer2_callback);  }
    if (handle->Instance == TIM3)  { CallbackElapsed(timer3_callback);  }
    if (handle->Instance == TIM4)  { CallbackElapsed(timer4_callback);  }
    if (handle->Instance == TIM5)  { CallbackElapsed(timer5_callback);  }
    if (handle->Instance == TIM9)  { CallbackElapsed(timer9_callback);  }
    if (handle->Instance == TIM10) { CallbackElapsed(timer10_callback); }
    if (handle->Instance == TIM11) { CallbackElapsed(timer11_callback); }
    if (handle->Instance == TIM12) { CallbackElapsed(timer12_callback); }
    if (handle->Instance == TIM13) { CallbackElapsed(timer13_callback); }
    if (handle->Instance == TIM14) { CallbackElapsed(timer14_callback); }
}

/**
 * \brief   ISR: route TIM2 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM2_IRQHandler(void)
{
    CallbackIRQ(timer2_callback);
}

/**
 * \brief   ISR: route TIM3 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM3_IRQHandler(void)
{
    CallbackIRQ(timer3_callback);
}

/**
 * \brief   ISR: route TIM4 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM4_IRQHandler(void)
{
    CallbackIRQ(timer4_callback);
}

/**
 * \brief   ISR: route TIM5 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM5_IRQHandler(void)
{
    CallbackIRQ(timer5_callback);
}

/**
 * \brief   ISR: route TIM9 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM1_BRK_TIM9_IRQHandler(void)
{
//    CallbackIRQ(timer1_callback);
    CallbackIRQ(timer9_callback);
}

/**
 * \brief   ISR: route TIM10 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM1_UP_TIM10_IRQHandler(void)
{
//    CallbackIRQ(timer1_callback);
    CallbackIRQ(timer10_callback);
}

/**
 * \brief   ISR: route TIM11 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
//    CallbackIRQ(timer1_callback);
    CallbackIRQ(timer11_callback);
}

/**
 * \brief   ISR: route TIM12 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM8_BRK_TIM12_IRQHandler(void)
{
//    CallbackIRQ(timer8_callback);
    CallbackIRQ(timer12_callback);
}

/**
 * \brief   ISR: route TIM13 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM8_UP_TIM13_IRQHandler(void)
{
//    CallbackIRQ(timer8_callback);
    CallbackIRQ(timer13_callback);
}

/**
 * \brief   ISR: route TIM14 interrupts to 'CallbackIRQ'.
 */
extern "C" void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
//    CallbackIRQ(timer8_callback);
    CallbackIRQ(timer14_callback);
}
