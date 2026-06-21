/**
 * \file    ScopedIrqMask.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   ScopedIrqMask
 *
 * \brief   RAII guard that masks one or two NVIC interrupt lines for the
 *          lifetime of the object.
 *
 * \details Disables the given IRQ line(s) on construction and re-enables them
 *          (in reverse order) on destruction, so a short critical section is
 *          released on every exit path -- including early returns. The driver
 *          async starters use it to arm a shared std::function callback slot
 *          atomically with respect to the peripheral ISR: mask, start the HAL
 *          transfer, assign the handler only on success, unmask on scope exit.
 *
 * \note    Intended to bracket a brief section on a line that is already
 *          enabled (the driver enables its IRQ during Init); the guard
 *          unconditionally re-enables on destruction. Not nestable on the same
 *          line with save/restore semantics -- it enables, it does not restore.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/utility/ScopedIrqMask
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef SCOPED_IRQ_MASK_HPP_
#define SCOPED_IRQ_MASK_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class ScopedIrqMask final
{
public:
    /**
     * \brief   Mask a single NVIC interrupt line for this object's lifetime.
     * \param   irq     The IRQ line to disable now and re-enable on destruction.
     */
    explicit ScopedIrqMask(IRQn_Type irq) :
        mIrqs{ irq, irq },
        mCount(1)
    {
        HAL_NVIC_DisableIRQ(irq);
    }

    /**
     * \brief   Mask two NVIC interrupt lines for this object's lifetime.
     * \param   irq0    The first IRQ line (e.g. I2C Event).
     * \param   irq1    The second IRQ line (e.g. I2C Error).
     * \details Both lines are disabled now and re-enabled (irq1 then irq0) on
     *          destruction.
     */
    ScopedIrqMask(IRQn_Type irq0, IRQn_Type irq1) :
        mIrqs{ irq0, irq1 },
        mCount(2)
    {
        HAL_NVIC_DisableIRQ(irq0);
        HAL_NVIC_DisableIRQ(irq1);
    }

    /** \brief Re-enable the masked line(s) in reverse order. */
    ~ScopedIrqMask()
    {
        for (uint8_t i = mCount; i > 0; --i)
        {
            HAL_NVIC_EnableIRQ(mIrqs[i - 1]);
        }
    }

    // Explicit disabled constructors/operators
    ScopedIrqMask(const ScopedIrqMask&)            = delete;
    ScopedIrqMask& operator=(const ScopedIrqMask&) = delete;
    ScopedIrqMask(ScopedIrqMask&&)                 = delete;
    ScopedIrqMask& operator=(ScopedIrqMask&&)      = delete;

private:
    IRQn_Type mIrqs[2];     ///< The masked line(s); only the first mCount are valid.
    uint8_t   mCount;       ///< Number of masked lines (1 or 2).
};


#endif  // SCOPED_IRQ_MASK_HPP_
