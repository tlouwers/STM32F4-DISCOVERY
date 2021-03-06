/**
 * \file    BasicTimer.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   BasicTimer
 *
 * \brief   BasicTimer class used for the DAC to drive the DMA based output sampling.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/BasicTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

#ifndef BASIC_TIMER_HPP_
#define BASIC_TIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IBasicTimer.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    BasicTimerInstance
 * \brief   Available BasicTimer instances.
 */
enum class BasicTimerInstance : uint8_t
{
    TIMER_6 = 6,
    TIMER_7 = 7
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  BasicTimerCallback
 * \brief   Data structure to contain callback for a BasicTimer instance.
 */
struct BasicTimerCallback {
    std::function<void()> callbackIRQ = nullptr;    ///< Callback to call when IRQ occurs.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class BasicTimer final : public IBasicTimer, public IConfigInitable
{
public:
    /**
     * \struct  Config
     * \brief   Configuration struct for BasicTimer.
     * \note    Can fine-tune frequency in the Init() and CalculatePeriod() methods.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the BasicTimer configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   frequency           Frequency of the timer in Hz. Range [20..65535] Hz.
         */
        Config(uint8_t interruptPriority,
               uint16_t frequency) :
            mInterruptPriority(interruptPriority),
            mFrequency(frequency)
        { }

        uint8_t  mInterruptPriority;    ///< Interrupt priority.
        uint16_t mFrequency;            ///< Frequency in Hz.
    };

    explicit BasicTimer(const BasicTimerInstance& instance);
    virtual ~BasicTimer();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool Start() override;
    bool IsStarted() const override;
    bool Stop() override;

private:
    BasicTimerInstance  mInstance;
    TIM_HandleTypeDef   mHandle = {};
    BasicTimerCallback& mBasicTimerCallback;
    bool                mInitialized;
    bool                mStarted;

    void SetInstance(const BasicTimerInstance& instance);
    void CheckAndEnableAHB1PeripheralClock(const BasicTimerInstance& instance);
    void CheckAndDisableAHB1PeripheralClock(const BasicTimerInstance& instance);
    uint16_t CalculatePeriod(uint16_t desiredFrequency);
    IRQn_Type GetIRQn(const BasicTimerInstance& instance);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ();
};


#endif  // BASIC_TIMER_HPP_
