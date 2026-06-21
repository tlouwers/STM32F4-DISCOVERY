/**
 * \file    TestSPI.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the SPI driver. Exercises the real driver
 *          (drivers/drivers/SPI) against the fake HAL SPI surface
 *          (tests/Fake/stm32f4xx_hal_spi.{h,cpp}) and the shared fake HAL DMA
 *          surface (used to wire a DMA stream into the Tx/Rx slots).
 *
 * \details Coverage focuses on the driver's own logic: config-id and bus-speed
 *          validation in Init(), the IsInit/Sleep lifecycle, the LinkDma
 *          direction routing, the parameter/state guards in front of every
 *          blocking / interrupt / DMA transfer (and their HAL-failure
 *          branches), and the SPIx_IRQn vector -> CallbackIRQ() ->
 *          HAL_SPI_IRQHandler dispatch incl. the post-Sleep no-dispatch case.
 *          The register-level HAL behaviour itself is hardware-only and not
 *          unit-tested.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"

// Test subject -- the real SPI driver.
#include "drivers/SPI/SPI.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_spi.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs these C-linkage ISR entry points; the test invokes them
// directly to prove each SPIx vector dispatches into the driver's CallbackIRQ().
extern "C" void SPI1_IRQHandler(void);
extern "C" void SPI2_IRQHandler(void);
extern "C" void SPI3_IRQHandler(void);


namespace {


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from SPI::Config::Id(), used to
// prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

// A valid SPI config: SPI1 runs on PCLK2 (84 MHz in the fake), so 1 MHz is well
// within range.
SPI::Config ValidConfig()
{
    return SPI::Config(0, SPI::Mode::_0, 1000000U);
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class SPI_Test : public ::testing::Test
{
protected:
    SPI_Test()
        : mSubject(SPIInstance::SPI_1)
    {
        FakeSPI_Reset();
        FakeDMA_Reset();
    }

    // Configure a DMA in the given direction so it can be linked into the SPI.
    bool ConfigureDma(DMA& dma, DMA::Direction direction)
    {
        return dma.Configure(DMA::Channel::Channel0, direction, DMA::BufferMode::Normal);
    }

    SPI mSubject;
};


/************************************************************************/
/* Init / IsInit / Sleep                                                */
/************************************************************************/
TEST_F(SPI_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(SPI_Test, Init_WrongConfigType_ReturnsFalse)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Init_BusSpeedZero_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Init(SPI::Config(0, SPI::Mode::_0, 0U)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Init_BusSpeedAbovePeripheralClock_ReturnsFalse)
{
    // SPI1 peripheral clock is 84 MHz in the fake; ask for more than that.
    EXPECT_FALSE(mSubject.Init(SPI::Config(0, SPI::Mode::_0, 90000000U)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeSPI_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(SPI_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeSPI_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}


/************************************************************************/
/* LinkDma                                                              */
/************************************************************************/
TEST_F(SPI_Test, LinkDma_UnconfiguredDma_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma2_Stream3);   // never Configure()'d
    EXPECT_FALSE(mSubject.LinkDma(dma));
}

TEST_F(SPI_Test, LinkDma_MemoryToPeripheral_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma2_Stream3);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(SPI_Test, LinkDma_PeripheralToMemory_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma2_Stream0);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::PeripheralToMemory));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(SPI_Test, LinkDma_MemoryToMemory_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma2_Stream1);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToMemory));

    EXPECT_FALSE(mSubject.LinkDma(dma));
}


/************************************************************************/
/* Blocking transfers                                                   */
/************************************************************************/
TEST_F(SPI_Test, WriteBlocking_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(SPI_Test, WriteBlocking_NullSrc_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.WriteBlocking(nullptr, 4));
}

TEST_F(SPI_Test, WriteBlocking_ZeroLength_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, 0));
}

TEST_F(SPI_Test, WriteBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(SPI_Test, WriteBlocking_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeSPI_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(SPI_Test, ReadBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadBlocking(dest, sizeof(dest)));
}

TEST_F(SPI_Test, ReadBlocking_NotInit_ReturnsFalse)
{
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadBlocking(dest, sizeof(dest)));
}

TEST_F(SPI_Test, WriteReadBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_TRUE(mSubject.WriteReadBlocking(src, dest, sizeof(src)));
}

TEST_F(SPI_Test, WriteReadBlocking_NullDest_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteReadBlocking(src, nullptr, sizeof(src)));
}


/************************************************************************/
/* Interrupt transfers                                                  */
/************************************************************************/
TEST_F(SPI_Test, WriteInterrupt_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteInterrupt(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteInterrupt(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, ReadInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadInterrupt(dest, sizeof(dest), nullptr));
}

TEST_F(SPI_Test, WriteReadInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_TRUE(mSubject.WriteReadInterrupt(src, dest, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteInterrupt_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeSPI_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteInterrupt(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, ReadInterrupt_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeSPI_SetTransferResult(HAL_ERROR);

    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadInterrupt(dest, sizeof(dest), nullptr));
}

TEST_F(SPI_Test, WriteReadInterrupt_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeSPI_SetTransferResult(HAL_ERROR);

    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_FALSE(mSubject.WriteReadInterrupt(src, dest, sizeof(src), nullptr));
}


/************************************************************************/
/* DMA transfers                                                        */
/************************************************************************/
TEST_F(SPI_Test, WriteDMA_NoTxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDMA(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteDMA_NotInit_ReturnsFalse)
{
    DMA tx(DMA::Stream::Dma2_Stream3);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDMA(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteDMA_TxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream3);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteDMA(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteDMA_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream3);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));
    FakeSPI_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDMA(src, sizeof(src), nullptr));
}

TEST_F(SPI_Test, ReadDMA_NoRxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDMA(dest, sizeof(dest), nullptr));
}

TEST_F(SPI_Test, ReadDMA_RxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma2_Stream0);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));

    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadDMA(dest, sizeof(dest), nullptr));
}

TEST_F(SPI_Test, ReadDMA_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma2_Stream0);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));
    FakeSPI_SetTransferResult(HAL_ERROR);

    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDMA(dest, sizeof(dest), nullptr));
}

TEST_F(SPI_Test, WriteReadDMA_OnlyTxLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream3);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));

    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_FALSE(mSubject.WriteReadDMA(src, dest, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteReadDMA_BothLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream3);
    DMA rx(DMA::Stream::Dma2_Stream0);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(tx));
    ASSERT_TRUE(mSubject.LinkDma(rx));

    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_TRUE(mSubject.WriteReadDMA(src, dest, sizeof(src), nullptr));
}

TEST_F(SPI_Test, WriteReadDMA_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream3);
    DMA rx(DMA::Stream::Dma2_Stream0);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(tx));
    ASSERT_TRUE(mSubject.LinkDma(rx));
    FakeSPI_SetTransferResult(HAL_ERROR);

    const uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t       dest[4] = {};
    EXPECT_FALSE(mSubject.WriteReadDMA(src, dest, sizeof(src), nullptr));
}


/************************************************************************/
/* IRQ dispatch                                                         */
/************************************************************************/
TEST_F(SPI_Test, IRQHandler_AfterInit_DispatchesIntoHal)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_EQ(0, FakeSPI_IRQHandlerCallCount());

    // Firing the SPI1 vector must route through the registered callbackIRQ into
    // HAL_SPI_IRQHandler exactly once.
    SPI1_IRQHandler();

    EXPECT_EQ(1, FakeSPI_IRQHandlerCallCount());
}

TEST_F(SPI_Test, IRQHandler_AfterSleep_DoesNotDispatch)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Sleep());   // DisconnectCallbacks() drops the slot.

    SPI1_IRQHandler();

    EXPECT_EQ(0, FakeSPI_IRQHandlerCallCount());
}


/************************************************************************/
/* Instance coverage (SPI2 / SPI3)                                      */
/************************************************************************/
// Constructing + initialising SPI_2 / SPI_3 exercises the SPI2/SPI3 arms of
// SetInstance, CheckAndEnable/DisablePeripheralClock and GetIRQn (SPI2/3 run on
// PCLK1 = 42 MHz in the fake, so 1 MHz stays in range).
TEST_F(SPI_Test, Init_Spi2Instance_ReturnsTrueAndSleeps)
{
    SPI spi2(SPIInstance::SPI_2);
    EXPECT_TRUE(spi2.Init(ValidConfig()));
    EXPECT_TRUE(spi2.Sleep());
}

TEST_F(SPI_Test, Init_Spi3Instance_ReturnsTrueAndSleeps)
{
    SPI spi3(SPIInstance::SPI_3);
    EXPECT_TRUE(spi3.Init(ValidConfig()));
    EXPECT_TRUE(spi3.Sleep());
}

TEST_F(SPI_Test, IRQHandler_Spi2_DispatchesIntoHal)
{
    SPI spi2(SPIInstance::SPI_2);
    ASSERT_TRUE(spi2.Init(ValidConfig()));

    SPI2_IRQHandler();

    EXPECT_EQ(1, FakeSPI_IRQHandlerCallCount());
}

TEST_F(SPI_Test, IRQHandler_Spi3_DispatchesIntoHal)
{
    SPI spi3(SPIInstance::SPI_3);
    ASSERT_TRUE(spi3.Init(ValidConfig()));

    SPI3_IRQHandler();

    EXPECT_EQ(1, FakeSPI_IRQHandlerCallCount());
}


/************************************************************************/
/* Mode coverage (polarity / phase)                                     */
/************************************************************************/
// Each SPI mode maps to a distinct polarity/phase pair; initialising with every
// mode exercises both GetPolarity and GetPhase switch tables.
TEST_F(SPI_Test, Init_EveryMode_ReturnsTrue)
{
    const SPI::Mode modes[] = {
        SPI::Mode::_0, SPI::Mode::_1, SPI::Mode::_2, SPI::Mode::_3
    };
    for (const auto& mode : modes)
    {
        SPI spi(SPIInstance::SPI_1);
        EXPECT_TRUE(spi.Init(SPI::Config(0, mode, 1000000U)));
    }
}


/************************************************************************/
/* Prescaler bus-speed branches                                         */
/************************************************************************/
// CalculatePrescaler walks an if/else-if ladder over PCLK/busSpeed; sweep bus
// speeds (SPI1 PCLK2 = 84 MHz in the fake) to land in each prescaler bucket
// (/256 down to /2).
TEST_F(SPI_Test, Init_VariousBusSpeeds_AllSelectAValidPrescaler)
{
    const uint32_t busSpeeds[] = {
        300000U,    // 84/0.3 = 280  -> /256
        600000U,    // 140          -> /128
        1200000U,   // 70           -> /64
        2500000U,   // 33           -> /32
        5000000U,   // 16           -> /16
        10000000U,  // 8            -> /8
        20000000U,  // 4            -> /4
        42000000U   // 2            -> /2
    };
    for (const auto& speed : busSpeeds)
    {
        SPI spi(SPIInstance::SPI_1);
        EXPECT_TRUE(spi.Init(SPI::Config(0, SPI::Mode::_0, speed)));
    }
}


/************************************************************************/
/* Completion-callback dispatch                                         */
/************************************************************************/
// The Write*/Read*/WriteRead* IT paths register a user handler that the HAL
// fires on completion via HAL_SPI_TxCplt/RxCplt/TxRxCpltCallback. Iterate every
// instance so each callback's per-instance dispatch arm (SPI1/2/3) is covered.
TEST_F(SPI_Test, WriteInterrupt_TxComplete_EachInstance_FiresHandler)
{
    const SPIInstance insts[] = {
        SPIInstance::SPI_1, SPIInstance::SPI_2, SPIInstance::SPI_3
    };
    for (const auto& inst : insts)
    {
        FakeSPI_Reset();
        SPI spi(inst);
        ASSERT_TRUE(spi.Init(ValidConfig()));

        bool fired = false;
        const uint8_t src[4] = { 1, 2, 3, 4 };
        ASSERT_TRUE(spi.WriteInterrupt(src, sizeof(src), [&]{ fired = true; }));

        FakeSPI_FireTxCplt();
        EXPECT_TRUE(fired);
    }
}

TEST_F(SPI_Test, ReadInterrupt_RxComplete_EachInstance_FiresHandler)
{
    const SPIInstance insts[] = {
        SPIInstance::SPI_1, SPIInstance::SPI_2, SPIInstance::SPI_3
    };
    for (const auto& inst : insts)
    {
        FakeSPI_Reset();
        SPI spi(inst);
        ASSERT_TRUE(spi.Init(ValidConfig()));

        bool fired = false;
        uint8_t dest[4] = {};
        ASSERT_TRUE(spi.ReadInterrupt(dest, sizeof(dest), [&]{ fired = true; }));

        FakeSPI_FireRxCplt();
        EXPECT_TRUE(fired);
    }
}

TEST_F(SPI_Test, WriteReadInterrupt_TxRxComplete_EachInstance_FiresHandler)
{
    const SPIInstance insts[] = {
        SPIInstance::SPI_1, SPIInstance::SPI_2, SPIInstance::SPI_3
    };
    for (const auto& inst : insts)
    {
        FakeSPI_Reset();
        SPI spi(inst);
        ASSERT_TRUE(spi.Init(ValidConfig()));

        bool fired = false;
        const uint8_t src[4]  = { 1, 2, 3, 4 };
        uint8_t       dest[4] = {};
        ASSERT_TRUE(spi.WriteReadInterrupt(src, dest, sizeof(src), [&]{ fired = true; }));

        FakeSPI_FireTxRxCplt();
        EXPECT_TRUE(fired);
    }
}


} // namespace
