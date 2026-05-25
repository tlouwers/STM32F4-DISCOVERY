/**
 * \file     AppLogic.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   AppLogic
 *
 * \brief   Interface-injected application logic for the StandupCounter.
 *
 * \details Holds the testable behaviour split out of Application: the
 *          button-triggered standup countdown choreography (smiley, the long
 *          speaking wait, the per-second beep countdown with shrinking delays
 *          and a final long beep). It depends only on driver interfaces
 *          (IHI_M1388AR, IPWM, IPin) and on IDelay for every blocking wait, so
 *          it can be unit-tested with Mock drivers and a recording delay mock,
 *          while Application remains the thin composition root that constructs
 *          and wires the concrete drivers.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/target/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef APPLOGIC_HPP_
#define APPLOGIC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "interfaces/IHI_M1388AR.hpp"
#include "interfaces/IPWM.hpp"
#include "interfaces/IPin.hpp"
#include "interfaces/IDelay.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Testable application logic, injected with driver interfaces.
 */
class AppLogic final
{
public:
    AppLogic(IHI_M1388AR& matrix, IPWM& pwm, IPin& ledGreen, IPin& ledOrange, IPin& ledRed, IDelay& clock);

    void OnButtonPressed();
    void Process();

private:
    IHI_M1388AR& mMatrix;
    IPWM&        mPWM;
    IPin&        mLedGreen;
    IPin&        mLedOrange;
    IPin&        mLedRed;
    IDelay&      mClock;

    std::atomic<bool> mButtonPressed;
};


#endif  // APPLOGIC_HPP_
