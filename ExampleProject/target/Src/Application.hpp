/**
 * \file Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for Accelerometer demo.
 *
* \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/ExampleProject/target/Src
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        10-2019
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "components/LIS3DSH/LIS3DSH.hpp"
#include "drivers/DMA/DMA.hpp"
#include "drivers/Pin/Pin.hpp"
#include "drivers/SPI/SPI.hpp"
#include "utility/CpuWakeCounter/CpuWakeCounter.hpp"


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
    Pin mMotionInt1;
    Pin mMotionInt2;

    SPI mSPI;

    DMA mDMA_SPI_Tx;
    DMA mDMA_SPI_Rx;

    LIS3DSH mLIS3DSH;
	
    CpuWakeCounter mCpuWakeCounter;

    std::atomic<bool> mButtonPressed;
    std::atomic<bool> mMotionDataAvailable;
    uint8_t mMotionLength;

    void ButtonPressedCallback();
    void MotionDataReceived(uint8_t length);

    void GetUsedStack();
    void GetUsedHeap();
};


#endif  // APPLICATION_HPP_
