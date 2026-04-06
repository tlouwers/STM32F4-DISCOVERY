/**
 * \file    SPI_arbiter.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SPI_arbiter
 *
 * \brief   Arbiter class for SPI (master) implementation.
 *
 * \note    The ChipSelect must be toggled outside this driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/arbiters/SPI
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef SPI_ARBITER_HPP_
#define SPI_ARBITER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <functional>
#include "drivers/SPI/SPI.hpp"
#include "interfaces/IInitable.hpp"
#include "interfaces/ISPI.hpp"
#include "utility/CircularFifo/CircularFifo.hpp"


/************************************************************************/
/* Defines                                                              */
/************************************************************************/
/**
 * \def     SPI_ARBITER_BUFFER_SIZE
 * \brief   Size of the SPI Arbiter buffer.
 */
#define SPI_ARBITER_BUFFER_SIZE     4   // Tweak to get better results, usually 4


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class SPI_arbiter final : public ISPI, public IConfigInitable
{
public:
    explicit SPI_arbiter(const SPIInstance& instance);
    virtual ~SPI_arbiter();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    const SPI_HandleTypeDef* GetPeripheralHandle() const;
    DMA_HandleTypeDef*& GetDmaTxHandle();
    DMA_HandleTypeDef*& GetDmaRxHandle();

    bool WriteDMA(const uint8_t* src, uint16_t length, const std::function<void()>& handler) override;
    bool WriteReadDMA(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler) override;
    bool ReadDMA(uint8_t* dest, uint16_t length, const std::function<void()>& handler) override;

    bool WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler) override;
    bool WriteReadInterrupt(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler) override;
    bool ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void()>& handler) override;

    bool WriteBlocking(const uint8_t* src, uint16_t length) override;
    bool WriteReadBlocking(const uint8_t* src, uint8_t* dest, uint16_t length) override;
    bool ReadBlocking(uint8_t* dest, uint16_t length) override;

private:
    /**
     * \enum    RequestType
     * \brief   Available request types for SPI.
     */
    enum class RequestType : uint8_t
    {
        Invalid,
        WriteDMA,
        WriteReadDMA,
        ReadDMA,
        WriteInterrupt,
        WriteReadInterrupt,
        ReadInterrupt,
    };

    /**
     * \struct  ArbiterElementSpiMaster
     * \brief   Structure to contain administration items for the arbiter to
     *          delay read/write requests to the SPI bus.
     * \details This element can be queued for later use, it provides a
     *          grouping of useful variables for administration.
     */
    struct ArbiterElementSpiMaster {
        RequestType request_type            /** Indicate the request type */        = RequestType::Invalid;
        const uint8_t * src                 /** Pointer to the data (to) sent */    = nullptr;
        uint8_t * dest                      /** Pointer to the data received */     = nullptr;
        size_t length                       /** The length of a message */          = 0;
        std::function<void()> callbackDone  /** Callback to call when done */       = nullptr;
    };

    CircularFifo<ArbiterElementSpiMaster, SPI_ARBITER_BUFFER_SIZE> mBuffer;

    SPI               mSpiMaster;
    std::atomic<bool> mBusy;
    std::atomic_flag  mLock = ATOMIC_FLAG_INIT;

    void DataRequestHandler();
};


#endif  // SPI_ARBITER_HPP_
