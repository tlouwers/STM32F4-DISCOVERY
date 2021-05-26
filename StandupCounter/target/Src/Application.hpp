/**
 * \file     Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for StandupCounter.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/StandupCounter/target/Src
 *
 * \details StandupCounter with 8x8 LED display and buzzer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "components/HI-M1388AR/HI-M1388AR.hpp"
#include "drivers/DMA/DMA.hpp"
#include "drivers/Pin/Pin.hpp"
#include "drivers/PWM/PWM.hpp"
#include "drivers/SPI/SPI.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Main application class.
 */
class Application
{
public:
    Application();
    virtual ~Application() {};

    bool Init();
    void Process();
    void Error();

private:
    Pin mButton;
    Pin mLedGreen;
    Pin mLedOrange;
    Pin mLedRed;
    Pin mLedBlue;
    Pin mChipSelect;
    Pin mPWMOut;

    PWM        mPWM;
    SPI        mSPI;
    HI_M1388AR mMatrix;

    DMA mDMA_SPI_Tx;
    DMA mDMA_SPI_Rx;

    std::atomic<bool> mButtonPressed;

    void ButtonPressedCallback();
};


#endif  // APPLICATION_HPP_
