/**
 * \file    Usart.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Usart
 *
 * \brief   USART peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/Usart
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.2
 * \date    05-2021
 */

#ifndef USART_HPP_
#define USART_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IUSART.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    UsartInstance
 * \brief   Available USART instances.
 */
enum class UsartInstance : uint8_t
{
    USART_1 = 1,
    USART_2 = 2,
    USART_3 = 3,
    USART_6 = 6
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  UsartCallbacks
 * \brief   Data structure to contain callbacks for an Usart instance.
 */
struct UsartCallbacks {
    std::function<void()> callbackIRQ = nullptr;          ///< Callback to call when IRQ occurs.
    std::function<void()> callbackTx  = nullptr;          ///< Callback to call when Tx done.
    std::function<void(uint16_t)> callbackRx  = nullptr;  ///< Callback to call when Rx done.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Usart final : public IUSART, public IConfigInitable
{
public:
    /**
     * \enum    Baudrate
     * \brief   Available USART baud rates.
     */
    enum class Baudrate : uint32_t
    {
        _9600  =    9600,     ///<   9600
        _19200 =   19200,     ///<  19200
        _38400 =   38400,     ///<  38400
        _57600 =   57600,     ///<  57400
        _115K2 =  115200,     ///< 115200
        _230K4 =  230400,     ///< 230400
        _460K8 =  460800,     ///< 460800
        _912K6 =  912600      ///< 912600
    };

    /**
     * \enum    WordLength
     * \brief   Available USART word lengths.
     */
    enum class WordLength : bool
    {
        _8_BIT,     ///< Default
        _9_BIT
    };

    /**
     * \enum    Parity
     * \brief   Parity settings for USART.
     */
    enum class Parity : uint8_t
    {
        EVEN,       ///< Even parity
        ODD,        ///< Odd parity
        NO          ///< No parity
    };

    /**
     * \enum    StopBits
     * \brief   Available USART stop bit modes.
     */
    enum class StopBits : bool
    {
        _1_BIT,
        _2_BIT
    };

    /**
     * \enum    OverSampling
     * \brief   Available USART over sampling modes.
     */
    enum class OverSampling : bool
    {
        _8_TIMES,   ///< Default
        _16_TIMES,
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for USART.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the USART configuration struct.
         * \param   interruptPriority       Priority of the interrupt.
         * \param   useHardwareFlowControl  Flag indicating to use hardware flow control (CTS/RTS).
         * \param   baudrate                Baud rate of the USART.
         * \param   wordLength              Word length of the USART         -- default: 8 bits.
         * \param   parity                  Parity of the USART              -- default: no parity.
         * \param   stopBits                Stop bit mode for the USART      -- default: 1 bit.
         * \param   overSampling            Over sampling mode for the USART -- default: 8 times.
         */
        Config(uint8_t interruptPriority,
               bool useHardwareFlowControl,
               Baudrate baudrate,
               WordLength wordLength = WordLength::_8_BIT,
               Parity parity = Parity::NO,
               StopBits stopBits = StopBits::_1_BIT,
               OverSampling overSampling = OverSampling::_8_TIMES) :
            mInterruptPriority(interruptPriority),
            mUseHardwareFlowControl(useHardwareFlowControl),
            mBaudrate(baudrate),
            mWordLength(wordLength),
            mParity(parity),
            mStopBits(stopBits),
            mOverSampling(overSampling)
        { }

        uint8_t      mInterruptPriority;        ///< Interrupt priority.
        bool         mUseHardwareFlowControl;   ///< Flag indicating to use hardware flow control.
        Baudrate     mBaudrate;                 ///< Baud rate of the USART.
        WordLength   mWordLength;               ///< Word length of the USART.
        Parity       mParity;                   ///< Parity of the USART.
        StopBits     mStopBits;                 ///< Stop bit mode for the USART.
        OverSampling mOverSampling;             ///< Over sampling of the USART.
    };


    explicit Usart(const UsartInstance& instance);
    virtual ~Usart();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    const UART_HandleTypeDef* GetPeripheralHandle() const;
    DMA_HandleTypeDef*& GetDmaTxHandle();
    DMA_HandleTypeDef*& GetDmaRxHandle();

    bool WriteDma(const uint8_t* src, uint16_t length, const std::function<void()>& handler) override;
    bool ReadDma(uint8_t* dest, uint16_t length, const std::function<void(uint16_t)>& handler, bool useIdleDetection = true) override;

    bool WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler) override;
    bool ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void(uint16_t)>& handler, bool useIdleDetection = true) override;

    bool WriteBlocking(const uint8_t* src, uint16_t length) override;
    bool ReadBlocking(uint8_t* dest, uint16_t length) override;

private:
    UsartInstance      mInstance;
    UART_HandleTypeDef mHandle = {};
    UsartCallbacks&    mUsartCallbacks;
    bool               mInitialized;

    void SetInstance(const UsartInstance& instance);
    void CheckAndEnableAHB1PeripheralClock(const UsartInstance& instance);
    void CheckAndDisableAHB1PeripheralClock(const UsartInstance& instance);
    uint32_t GetParity(const Parity& parity);
    IRQn_Type GetIRQn(const UsartInstance& instance);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ();
};


#endif  // USART_HPP_
