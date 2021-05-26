/**
 * \file    I2C.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   I2C
 *
 * \brief   I2C master peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/I2C
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

#ifndef I2C_HPP_
#define I2C_HPP_

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
 * \enum    I2CInstance
 * \brief   Available I2C instances.
 */
enum class I2CInstance : uint8_t
{
    I2C_1 = 1,
    I2C_2 = 2,
    I2C_3 = 3
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  I2CCallbacks
 * \brief   Data structure to contain callbacks for an I2C instance.
 */
struct I2CCallbacks {
    std::function<void()> callbackEvent = nullptr;  ///< Callback to call when Event occurs.
    std::function<void()> callbackError = nullptr;  ///< Callback to call when Error occurs.
    std::function<void()> callbackTx    = nullptr;  ///< Callback to call when Tx done.
    std::function<void()> callbackRx    = nullptr;  ///< Callback to call when Rx done.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   I2C peripheral driver class.
 */
class I2C
{
public:
    /**
     * \enum    BusSpeed
     * \brief   Available I2C bus speeds.
     */
    enum class BusSpeed : bool
    {
        NORMAL,     ///< 100 kHz
        FAST        ///< 400 kHz
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for I2C.
     */
    struct Config
    {
        /**
         * \brief   Constructor of the I2C configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   busSpeed            The speed of the I2C bus.
         */
        Config(uint8_t interruptPriority,
               BusSpeed busSpeed) :
            mInterruptPriority(interruptPriority),
            mBusSpeed(busSpeed)
        { }

        uint8_t  mInterruptPriority;    ///< Interrupt priority.
        BusSpeed mBusSpeed;             ///< Speed of the bus.
    };

    explicit I2C(const I2CInstance& instance);
    virtual ~I2C();

    bool Init(const Config& config);
    bool IsInit() const;
    bool Sleep();

    const I2C_HandleTypeDef* GetPeripheralHandle() const;
    DMA_HandleTypeDef*& GetDmaTxHandle();
    DMA_HandleTypeDef*& GetDmaRxHandle();

    bool WriteDMA(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler);
    bool ReadDMA(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler);

    bool WriteInterrupt(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler);
    bool ReadInterrupt(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler);

    bool WriteBlocking(uint8_t slave, const uint8_t* src, uint16_t length);
    bool ReadBlocking(uint8_t slave, uint8_t* dest, uint16_t length);

private:
    /**
     * \enum    IRQType
     * \brief   Available IRQ types.
     */
    enum class IRQType : bool
    {
        Event,      ///< Default
        Error
    };

    I2CInstance       mInstance;
    I2C_HandleTypeDef mHandle = {};
    I2CCallbacks&     mI2CCallbacks;
    bool              mInitialized;

    void SetInstance(const I2CInstance& instance);
    void CheckAndEnableAHB1PeripheralClock(const I2CInstance& instance);
    void CheckAndDisableAHB1PeripheralClock(const I2CInstance& instance);
    IRQn_Type GetIRQn(const I2CInstance& instance, IRQType type);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackEvent();
    void CallbackError();
};


#endif  // I2C_HPP_
