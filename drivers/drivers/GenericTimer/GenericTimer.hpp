/**
 * \file    GenericTimer.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   GenericTimer
 *
 * \brief   Helper class to provide general elapsed timer functionality.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/GenericTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef GENERIC_TIMER_HPP_
#define GENERIC_TIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IGenericTimer.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    GenericTimerInstance
 * \brief   Available GenericTimer instances.
 */
enum class GenericTimerInstance : uint8_t
{
    TIMER_2  =  2,
    TIMER_3  =  3,
    TIMER_4  =  4,
    TIMER_5  =  5,
    TIMER_9  =  9,
    TIMER_10 = 10,
    TIMER_11 = 11,
    TIMER_12 = 12,
    TIMER_13 = 13,
    TIMER_14 = 14
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  GenericTimerCallbacks
 * \brief   Data structure to contain callback for a GenericTimer instance.
 */
struct GenericTimerCallbacks {
    std::function<void()> callbackIRQ = nullptr;        ///< Callback to call when IRQ occurs.
    std::function<void()> callbackElapsed = nullptr;    ///< Callback to call when timer elapsed event occurs.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class GenericTimer final : public IGenericTimer, public IConfigInitable
{
public:
    /**
     * \struct  Config
     * \brief   Configuration struct for GenericTimer.
     * \note    Can fine-tune frequency in the Init() and CalculatePeriod() methods.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the GenericTimer configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   frequency           Frequency of the timer in Hz. Range [1..10000] Hz.
         */
        Config(uint8_t interruptPriority,
               uint16_t frequency) :
            mInterruptPriority(interruptPriority),
            mFrequency(frequency)
        { }

        uint8_t  mInterruptPriority;    ///< Interrupt priority.
        uint16_t mFrequency;            ///< Frequency in Hz.
    };

    explicit GenericTimer(const GenericTimerInstance& instance);
    virtual ~GenericTimer();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool Start(const std::function<void()>& handler) override;
    bool IsStarted() const override;
    bool Stop() override;

private:
    GenericTimerInstance   mInstance;
    TIM_HandleTypeDef      mHandle = {};
    GenericTimerCallbacks& mGenericTimerCallback;
    bool                   mInitialized;
    bool                   mStarted;

    void SetInstance(const GenericTimerInstance& instance);
    void CheckAndEnableAHBPeripheralClock(const GenericTimerInstance& instance);
    void CheckAndDisableAHBPeripheralClock(const GenericTimerInstance& instance);
    uint16_t CalculatePeriod(uint16_t desiredFrequency);
    IRQn_Type GetIRQn(const GenericTimerInstance& instance);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ();
};


#endif  // GENERIC_TIMER_HPP_
