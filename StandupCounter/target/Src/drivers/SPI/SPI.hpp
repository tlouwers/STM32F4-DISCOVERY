/**
 * \file    SPI.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SPI
 *
 * \brief   SPI peripheral driver class - Master only.
 *
 * \note    The ChipSelect must be toggled outside this driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/SPI
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

#ifndef SPI_HPP_
#define SPI_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    SPIInstance
 * \brief   Available SPI instances.
 */
enum class SPIInstance : uint8_t
{
    SPI_1 = 1,
    SPI_2 = 2,
    SPI_3 = 3
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  SPICallbacks
 * \brief   Data structure to contain callbacks for a SPI instance.
 */
struct SPICallbacks {
    std::function<void()> callbackIRQ  = nullptr;   ///< Callback to call when IRQ occurs.
    std::function<void()> callbackTxRx = nullptr;   ///< Callback to call when Tx/Rx done.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class SPI
{
public:
    /**
     * \enum    Mode
     * \brief   Available SPI modes (CPOL/CPHA).
     */
    enum class Mode : uint8_t
    {
        _0,     ///< CPOL = 0, CPHA = 0
        _1,     ///< CPOL = 0, CPHA = 1
        _2,     ///< CPOL = 1, CPHA = 0
        _3      ///< CPOL = 1, CPHA = 1
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for SPI.
     */
    struct Config
    {
        /**
         * \brief   Constructor of the SPI configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   mode                The mode of the SPI bus (CPOL/CPHA).
         * \param   busSpeed            The speed of the SPI bus.
         */
        Config(uint8_t interruptPriority, Mode mode, uint32_t busSpeed) :
            mInterruptPriority(interruptPriority),
            mMode(mode),
            mBusSpeed(busSpeed)
        { }

        uint8_t    mInterruptPriority;  ///< Interrupt priority.
        Mode       mMode;               ///< Clock polarity and phase.
        uint32_t   mBusSpeed;           ///< Speed of the bus.
    };

    explicit SPI(const SPIInstance& instance);
    virtual ~SPI();

    bool Init(const Config& config);
    bool IsInit() const;
    bool Sleep();

    const SPI_HandleTypeDef* GetPeripheralHandle() const;
    DMA_HandleTypeDef*& GetDmaTxHandle();
    DMA_HandleTypeDef*& GetDmaRxHandle();

    bool WriteDMA(const uint8_t* src, uint16_t length, const std::function<void()>& handler);
    bool WriteReadDMA(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler);
    bool ReadDMA(uint8_t* dest, uint16_t length, const std::function<void()>& handler);

    bool WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler);
    bool WriteReadInterrupt(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler);
    bool ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void()>& handler);

    bool WriteBlocking(const uint8_t* src, uint16_t length);
    bool WriteReadBlocking(const uint8_t* src, uint8_t* dest, uint16_t length);
    bool ReadBlocking(uint8_t* dest, uint16_t length);

private:
    SPIInstance       mInstance;
    SPI_HandleTypeDef mHandle = {};
    SPICallbacks&     mSPICallbacks;
    bool              mInitialized;

    void SetInstance(const SPIInstance& instance);
    void CheckAndEnableAHBPeripheralClock(const SPIInstance& instance);
    void CheckAndDisableAHBPeripheralClock(const SPIInstance& instance);
    uint32_t GetPolarity(const Mode& mode);
    uint32_t GetPhase(const Mode& mode);
    uint32_t CalculatePrescaler(uint32_t busSpeed);
    IRQn_Type GetIRQn(const SPIInstance& instance);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ() const;
};

#endif  // SPI_HPP_
