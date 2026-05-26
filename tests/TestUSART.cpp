/**
 * \file    TestUSART.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the USART driver. Exercises the real driver
 *          (drivers/drivers/USART) against the fake HAL UART surface
 *          (tests/Fake/stm32f4xx_hal_usart.{h,cpp}) and the shared fake HAL DMA
 *          surface (for Tx/Rx slot wiring).
 *
 * \details Coverage focuses on the driver's own logic: config-id validation +
 *          HAL-init failure in Init(); the IsInit/Sleep lifecycle; LinkDma
 *          direction routing; the not-init / no-DMA-linked guards and the
 *          HAL-delegated null/zero-length rejection of every blocking/
 *          interrupt/DMA transfer (incl. ReadDma with IDLE detection on/off);
 *          and the USARTx_IRQn -> CallbackIRQ -> HAL_UART_IRQHandler dispatch
 *          (non-IDLE path, since the fake status register reads 0) incl. the
 *          post-Sleep no-dispatch case.
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

// Test subject -- the real USART driver.
#include "drivers/USART/USART.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_usart.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs these C-linkage ISR entry points; the test fires them to
// prove each USARTx vector dispatches into CallbackIRQ -> HAL_UART_IRQHandler.
extern "C" void USART1_IRQHandler(void);
extern "C" void USART2_IRQHandler(void);
extern "C" void USART3_IRQHandler(void);
extern "C" void USART6_IRQHandler(void);


namespace {


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from USART::Config::Id().
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

USART::Config ValidConfig()
{
    return USART::Config(0, false, USART::Baudrate::_115K2);
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class USART_Test : public ::testing::Test
{
protected:
    USART_Test()
        : mSubject(UsartInstance::USART_1)
    {
        FakeUSART_Reset();
        FakeDMA_Reset();
    }

    bool ConfigureDma(DMA& dma, DMA::Direction direction)
    {
        return dma.Configure(DMA::Channel::Channel0, direction, DMA::BufferMode::Normal);
    }

    USART mSubject;
};


/************************************************************************/
/* Init / IsInit / Sleep                                                */
/************************************************************************/
TEST_F(USART_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(USART_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(USART_Test, Init_FullConfig_ReturnsTrue)
{
    // Exercise the non-default WordLength/Parity/StopBits/OverSampling + flow ctl.
    USART::Config cfg(1, true, USART::Baudrate::_9600,
                      USART::WordLength::_9_BIT, USART::Parity::EVEN,
                      USART::StopBits::_2_BIT, USART::OverSampling::_16_TIMES);
    EXPECT_TRUE(mSubject.Init(cfg));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(USART_Test, Init_WrongConfigType_ReturnsFalse)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(USART_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeUSART_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(USART_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(USART_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeUSART_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}


/************************************************************************/
/* LinkDma                                                              */
/************************************************************************/
TEST_F(USART_Test, LinkDma_UnconfiguredDma_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma2_Stream7);
    EXPECT_FALSE(mSubject.LinkDma(dma));
}

TEST_F(USART_Test, LinkDma_MemoryToPeripheral_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma2_Stream7);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(USART_Test, LinkDma_PeripheralToMemory_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma2_Stream2);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::PeripheralToMemory));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(USART_Test, LinkDma_MemoryToMemory_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma2_Stream1);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToMemory));

    EXPECT_FALSE(mSubject.LinkDma(dma));
}


/************************************************************************/
/* Blocking transfers                                                   */
/************************************************************************/
TEST_F(USART_Test, WriteBlocking_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(USART_Test, WriteBlocking_NullSrc_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.WriteBlocking(nullptr, 4));
}

TEST_F(USART_Test, WriteBlocking_ZeroLength_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, 0));
}

TEST_F(USART_Test, WriteBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(USART_Test, WriteBlocking_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeUSART_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(src, sizeof(src)));
}

TEST_F(USART_Test, ReadBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadBlocking(dest, sizeof(dest)));
}

TEST_F(USART_Test, ReadBlocking_NotInit_ReturnsFalse)
{
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadBlocking(dest, sizeof(dest)));
}


/************************************************************************/
/* Interrupt transfers                                                  */
/************************************************************************/
TEST_F(USART_Test, WriteInterrupt_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteInterrupt(src, sizeof(src), nullptr));
}

TEST_F(USART_Test, WriteInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteInterrupt(src, sizeof(src), nullptr));
}

TEST_F(USART_Test, ReadInterrupt_WithIdleDetection_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadInterrupt(dest, sizeof(dest), nullptr, true));
}

TEST_F(USART_Test, ReadInterrupt_WithoutIdleDetection_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadInterrupt(dest, sizeof(dest), nullptr, false));
}


/************************************************************************/
/* DMA transfers                                                        */
/************************************************************************/
TEST_F(USART_Test, WriteDma_NoTxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDma(src, sizeof(src), nullptr));
}

TEST_F(USART_Test, WriteDma_TxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma2_Stream7);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteDma(src, sizeof(src), nullptr));
}

TEST_F(USART_Test, ReadDma_NoRxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDma(dest, sizeof(dest), nullptr));
}

TEST_F(USART_Test, ReadDma_RxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma2_Stream2);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));

    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadDma(dest, sizeof(dest), nullptr, true));
}

TEST_F(USART_Test, ReadDma_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma2_Stream2);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));
    FakeUSART_SetTransferResult(HAL_ERROR);

    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDma(dest, sizeof(dest), nullptr, false));
}


/************************************************************************/
/* IRQ dispatch                                                         */
/************************************************************************/
TEST_F(USART_Test, IRQHandler_AfterInit_DispatchesIntoHal)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_EQ(0, FakeUSART_IRQHandlerCallCount());

    // SR reads 0, so CallbackIRQ takes the non-IDLE path straight into
    // HAL_UART_IRQHandler.
    USART1_IRQHandler();

    EXPECT_EQ(1, FakeUSART_IRQHandlerCallCount());
}

TEST_F(USART_Test, IRQHandler_AfterSleep_DoesNotDispatch)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Sleep());   // DisconnectCallbacks() drops the slot.

    USART1_IRQHandler();

    EXPECT_EQ(0, FakeUSART_IRQHandlerCallCount());
}


/************************************************************************/
/* Config coverage (parity)                                             */
/************************************************************************/
// ValidConfig uses Parity::NO and the FullConfig test uses EVEN; cover ODD so
// every GetParity switch arm is exercised.
TEST_F(USART_Test, Init_OddParity_ReturnsTrue)
{
    USART::Config cfg(0, false, USART::Baudrate::_115K2,
                      USART::WordLength::_8_BIT, USART::Parity::ODD);
    EXPECT_TRUE(mSubject.Init(cfg));
}


/************************************************************************/
/* Instance coverage (USART2 / USART3 / USART6)                         */
/************************************************************************/
// Constructing + initialising each instance exercises the USART2/3/6 arms of
// SetInstance, CheckAndEnable/DisablePeripheralClock and GetIRQn.
TEST_F(USART_Test, Init_EachInstance_ReturnsTrueAndSleeps)
{
    const UsartInstance insts[] = {
        UsartInstance::USART_2, UsartInstance::USART_3, UsartInstance::USART_6
    };
    for (const auto& inst : insts)
    {
        FakeUSART_Reset();
        USART u(inst);
        EXPECT_TRUE(u.Init(ValidConfig()));
        EXPECT_TRUE(u.Sleep());
    }
}


/************************************************************************/
/* Completion-callback dispatch                                         */
/************************************************************************/
// The Write* IT path registers a void() handler the HAL fires on Tx completion;
// iterate every instance so HAL_UART_TxCpltCallback's per-instance dispatch
// (USART1/2/3/6) is covered.
TEST_F(USART_Test, WriteInterrupt_TxComplete_EachInstance_FiresHandler)
{
    const UsartInstance insts[] = {
        UsartInstance::USART_1, UsartInstance::USART_2,
        UsartInstance::USART_3, UsartInstance::USART_6
    };
    for (const auto& inst : insts)
    {
        FakeUSART_Reset();
        USART u(inst);
        ASSERT_TRUE(u.Init(ValidConfig()));

        bool fired = false;
        const uint8_t src[4] = { 1, 2, 3, 4 };
        ASSERT_TRUE(u.WriteInterrupt(src, sizeof(src), [&]{ fired = true; }));

        FakeUSART_FireTxCplt();
        EXPECT_TRUE(fired);
    }
}

// The Rx-complete path runs through CallbackIRQ's IDLE branch: a registered
// ReadInterrupt arms RxXferSize; setting the IDLE flag + firing the instance's
// vector dispatches HAL_UART_RxCpltCallback -> the user handler with the byte
// count. Covers the IDLE branch, the RxCplt body, every per-instance dispatch
// arm and every USARTx vector.
TEST_F(USART_Test, ReadInterrupt_RxComplete_IdlePath_EachInstance_FiresHandler)
{
    const struct { UsartInstance inst; void (*isr)(void); } cases[] = {
        { UsartInstance::USART_1, &USART1_IRQHandler },
        { UsartInstance::USART_2, &USART2_IRQHandler },
        { UsartInstance::USART_3, &USART3_IRQHandler },
        { UsartInstance::USART_6, &USART6_IRQHandler },
    };
    for (const auto& c : cases)
    {
        FakeUSART_Reset();
        USART u(c.inst);
        ASSERT_TRUE(u.Init(ValidConfig()));

        bool     called = false;
        uint16_t got    = 0;
        uint8_t  dest[4] = {};
        ASSERT_TRUE(u.ReadInterrupt(dest, sizeof(dest),
                                    [&](uint16_t n){ called = true; got = n; }, true));

        FakeUSART_SetIdleFlag(1);
        c.isr();

        EXPECT_TRUE(called);
        EXPECT_EQ(4u, got);
    }
}


} // namespace
