/**
 * \file    HalTimer.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   HalTimer
 *
 * \brief   Helper class to provide timer tick to the STM32 HAL, based upon
 *          the GenericTimer class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/HalTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    12-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/HalTimer/HalTimer.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Static Variables                                                     */
/************************************************************************/
/**
 * \brief   Timer handle, used to suspend and resume the timer.
 */
static TIM_HandleTypeDef hTimer;


/************************************************************************/
/* Hal Timer Tick Override Functions                                    */
/************************************************************************/
/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling the hTimer update
  *         interrupt.
  */
void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&hTimer, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by enabling the hTimer update
  *         interrupt.
  */
void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&hTimer, TIM_IT_UPDATE);
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal HalTimer instance administration.
 * \param   instance    The GenericTimer instance to use for the HalTimer.
 */
HalTimer::HalTimer(const GenericTimerInstance& instance) :
    mGenericTimer(instance)
{
    SetInstance(instance);
}

/**
 * \brief   Destructor, stop timer, disabled interrupts.
 * \note    This will stop the HAL_IncTick as well.
 */
HalTimer::~HalTimer()
{
    mGenericTimer.Sleep();
}

/**
 * \brief   Initializes and starts the HalTimer instance with the given
 *          configuration.
 * \param   config  The configuration for the HalTimer instance to use.
 * \returns True if the configuration could be applied and the timer could be
 *          started, else false.
 */
bool HalTimer::Init(const IConfig& config)
{
    const Config& cfg = reinterpret_cast<const Config&>(config);

    if (mGenericTimer.Init(GenericTimer::Config(TICK_INT_PRIORITY, cfg.mFrequency)))
    {
        return mGenericTimer.Start([this]() { this->HalTimerTick(); });
    }
    return false;
}

/**
 * \brief   Indicate if HalTimer is initialized.
 * \returns True if HalTimer is initialized, else false.
 */
bool HalTimer::IsInit() const
{
    return mGenericTimer.IsInit();
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the HalTimer instance into internal administration.
 * \param   instance    The HalTimer instance to use.
 * \note    Asserts if the HalTimer instance is invalid.
 */
void HalTimer::SetInstance(const GenericTimerInstance& instance)
{
    switch (instance)
    {
        case GenericTimerInstance::TIMER_2:  hTimer.Instance = TIM2;  break;
        case GenericTimerInstance::TIMER_3:  hTimer.Instance = TIM3;  break;
        case GenericTimerInstance::TIMER_4:  hTimer.Instance = TIM4;  break;
        case GenericTimerInstance::TIMER_5:  hTimer.Instance = TIM5;  break;
        case GenericTimerInstance::TIMER_9:  hTimer.Instance = TIM9;  break;
        case GenericTimerInstance::TIMER_10: hTimer.Instance = TIM10; break;
        case GenericTimerInstance::TIMER_11: hTimer.Instance = TIM11; break;
        case GenericTimerInstance::TIMER_12: hTimer.Instance = TIM12; break;
        case GenericTimerInstance::TIMER_13: hTimer.Instance = TIM13; break;
        case GenericTimerInstance::TIMER_14: hTimer.Instance = TIM14; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Increment the Hal_IncTick.
 */
void HalTimer::HalTimerTick()
{
    HAL_IncTick();
}
