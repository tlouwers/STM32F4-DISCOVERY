/**
 * \file    IAdc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for Adc peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IADC_HPP_
#define IADC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IAdc
{
public:
    virtual ~IAdc() = default;

    /**
     * \brief   Sample a single value with the ADC (blocking / polling).
     * \param   value   Output: set to the sampled raw ADC value on success.
     * \returns True if sampling succeeded and \p value was set, else false.
     */
    virtual bool GetValue(uint16_t& value) = 0;

    /**
     * \brief   Start an interrupt-driven ADC sample; the result is delivered to
     *          the handler when the conversion completes.
     * \param   handler   Callback invoked with the raw ADC value on completion.
     * \returns True if the conversion could be started, else false.
     * \note    The handler runs in INTERRUPT context (the ADC conversion-complete
     *          ISR). Keep it short and call only ISR-safe operations; do not
     *          block or call non-ISR-safe RTOS APIs (use the ...FromISR variants).
     */
    virtual bool GetValueInterrupt(const std::function<void(uint16_t)>& handler) = 0;
};


#endif  // IADC_HPP_
