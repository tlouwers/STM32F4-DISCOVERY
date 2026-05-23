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
// The driver installs this C-linkage ISR entry point; the test invokes it
// directly to prove the SPI1 vector dispatches into the driver's CallbackIRQ().
extern "C" void SPI1_IRQHandler(void);


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


} // namespace
