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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/BasicTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

#ifndef BASICTIMER_HPP_
#define BASICTIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IBasicTimer.hpp"
#include "drivers/TimerIRQ/TimerIRQ.hpp"
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
         * \param   frequency           Frequency of the timer in Hz. Must be > 0.
         *                              The usable range is ~16..65535 Hz: TIM6/TIM7
         *                              are 16-bit with a 1 MHz CK_CNT, so any value
         *                              below 1 MHz/65536 (~15.26 Hz) saturates to the
         *                              lowest representable frequency (ARR = 0xFFFF).
         *                              See CalculatePeriod().
         */
        Config(uint8_t interruptPriority,
               uint16_t frequency) :
            mInterruptPriority(interruptPriority),
            mFrequency(frequency)
        { }

        uint8_t  mInterruptPriority;    ///< Interrupt priority.
        uint16_t mFrequency;            ///< Frequency in Hz.

        /**
         * \brief   Unique runtime type tag for this Config.
         * \returns Address stable and unique to this Config type.
         */
        static const void* Id() { static const char sTag = 0; return &sTag; }

        /** \brief Runtime type identity, see IConfig::ConfigId(). */
        const void* ConfigId() const override { return Id(); }
    };

    explicit BasicTimer(const BasicTimerInstance& instance);
    virtual ~BasicTimer();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    /**
     * \brief   Enables the timer counter (CR1.CEN only — no update IRQ).
     * \note    The TimerIRQ slot for TIM6/TIM7 is installed at Init() and
     *          torn down at Sleep()/dtor, but UDIE is never enabled and the
     *          slot's handler is never invoked. The only documented consumer
     *          is the DAC TRGO chain (CPU-less), so no periodic CPU callback
     *          is required.
     *
     *          To activate the slot in the future:
     *            1. Add `bool RegisterCallback(const std::function<void()>&)`
     *               to `IBasicTimer` and a matching override here.
     *            2. Switch this `HAL_TIM_Base_Start` call to
     *               `HAL_TIM_Base_Start_IT` (enables UDIE).
     *            3. Have the TimerIRQ slot handler invoke the registered
     *               callback.
     *          The ISR seam already exists; only the API surface + the
     *          Start variant change are required. Deliberately deferred —
     *          see driver_update.md "BasicTimer ### 1" and ledger L5b.
     * \returns True if started, else false.
     */
    bool Start() override;
    bool IsStarted() const override;
    bool Stop() override;

    // Explicit disabled constructors/operators
    BasicTimer(const BasicTimer&)            = delete;
    BasicTimer& operator=(const BasicTimer&) = delete;
    BasicTimer(BasicTimer&&)                 = delete;
    BasicTimer& operator=(BasicTimer&&)      = delete;

private:
    BasicTimerInstance  mInstance;
    TIM_HandleTypeDef   mHandle = {};
    bool                mInitialized;
    bool                mStarted;

    void SetInstance(const BasicTimerInstance& instance);
    void CheckAndEnablePeripheralClock(const BasicTimerInstance& instance);
    void CheckAndDisablePeripheralClock(const BasicTimerInstance& instance);
    static uint32_t GetTimerInputClockFreq();
    uint16_t CalculatePeriod(uint16_t desiredFrequency);
    IRQn_Type GetIRQn(const BasicTimerInstance& instance);
    TimerIRQ::Slot GetSlot(const BasicTimerInstance& instance);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void DisconnectCallbacks();
};


#endif  // BASICTIMER_HPP_
