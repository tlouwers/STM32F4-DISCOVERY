/**
 * \file    TestI2C.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the I2C driver. Exercises the real driver
 *          (drivers/drivers/I2C) against the fake HAL I2C surface
 *          (tests/Fake/stm32f4xx_hal_i2c.{h,cpp}), the shared fake HAL DMA
 *          surface (for Tx/Rx slot wiring) and the fake Pin driver (used by the
 *          bit-banged bus recovery).
 *
 * \details Coverage focuses on the driver's own logic: config-id validation +
 *          HAL-init failure in Init(); the IsInit/Sleep lifecycle incl. the
 *          busy-state abort branch; LinkDma direction routing; the
 *          parameter/state/no-DMA guards in front of every blocking/interrupt/
 *          DMA transfer (and their HAL-failure branches); the RecoverBus
 *          guard + that the bit-bang sequence runs to completion (the fake DWT
 *          delay must terminate) and re-initialises the peripheral; and the
 *          I2Cx_EV/ER_IRQn -> CallbackEvent/Error -> HAL_I2C_*_IRQHandler
 *          dispatch incl. the post-Sleep no-dispatch case.
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

// Test subject -- the real I2C driver.
#include "drivers/I2C/I2C.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_i2c.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The driver installs these C-linkage ISR entry points; the test fires them
// directly to prove each I2Cx vector dispatches into CallbackEvent/Error.
extern "C" void I2C1_EV_IRQHandler(void);
extern "C" void I2C1_ER_IRQHandler(void);
extern "C" void I2C2_EV_IRQHandler(void);
extern "C" void I2C2_ER_IRQHandler(void);
extern "C" void I2C3_EV_IRQHandler(void);
extern "C" void I2C3_ER_IRQHandler(void);


namespace {


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from I2C::Config::Id().
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

I2C::Config ValidConfig()
{
    return I2C::Config(0, I2C::BusSpeed::NORMAL);
}

constexpr uint8_t kSlave = 0x50;


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class I2C_Test : public ::testing::Test
{
protected:
    I2C_Test()
        : mSubject(I2CInstance::I2C_1)
    {
        FakeI2C_Reset();
        FakeDMA_Reset();
    }

    bool ConfigureDma(DMA& dma, DMA::Direction direction)
    {
        return dma.Configure(DMA::Channel::Channel0, direction, DMA::BufferMode::Normal);
    }

    I2C mSubject;
};


/************************************************************************/
/* Init / IsInit / Sleep                                                */
/************************************************************************/
TEST_F(I2C_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(I2C_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(I2C_Test, Init_FastBusSpeed_ReturnsTrue)
{
    EXPECT_TRUE(mSubject.Init(I2C::Config(0, I2C::BusSpeed::FAST)));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(I2C_Test, Init_WrongConfigType_ReturnsFalse)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(I2C_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeI2C_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(I2C_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(I2C_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeI2C_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}

TEST_F(I2C_Test, Sleep_BusyTxState_AbortsInFlightTransfer)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeI2C_SetState(HAL_I2C_STATE_BUSY_TX);

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_EQ(1, FakeI2C_AbortCallCount());
}

TEST_F(I2C_Test, Sleep_ReadyState_DoesNotAbort)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));   // state defaults to READY

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_EQ(0, FakeI2C_AbortCallCount());
}


/************************************************************************/
/* LinkDma                                                              */
/************************************************************************/
TEST_F(I2C_Test, LinkDma_UnconfiguredDma_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma1_Stream6);
    EXPECT_FALSE(mSubject.LinkDma(dma));
}

TEST_F(I2C_Test, LinkDma_MemoryToPeripheral_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma1_Stream6);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToPeripheral));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(I2C_Test, LinkDma_PeripheralToMemory_ReturnsTrue)
{
    DMA dma(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::PeripheralToMemory));

    EXPECT_TRUE(mSubject.LinkDma(dma));
}

TEST_F(I2C_Test, LinkDma_MemoryToMemory_ReturnsFalse)
{
    DMA dma(DMA::Stream::Dma1_Stream0);
    ASSERT_TRUE(ConfigureDma(dma, DMA::Direction::MemoryToMemory));

    EXPECT_FALSE(mSubject.LinkDma(dma));
}


/************************************************************************/
/* Blocking transfers                                                   */
/************************************************************************/
TEST_F(I2C_Test, WriteBlocking_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(kSlave, src, sizeof(src)));
}

TEST_F(I2C_Test, WriteBlocking_NullSrc_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.WriteBlocking(kSlave, nullptr, 4));
}

TEST_F(I2C_Test, WriteBlocking_ZeroLength_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(kSlave, src, 0));
}

TEST_F(I2C_Test, WriteBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteBlocking(kSlave, src, sizeof(src)));
}

TEST_F(I2C_Test, WriteBlocking_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeI2C_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteBlocking(kSlave, src, sizeof(src)));
}

TEST_F(I2C_Test, ReadBlocking_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadBlocking(kSlave, dest, sizeof(dest)));
}

TEST_F(I2C_Test, ReadBlocking_NotInit_ReturnsFalse)
{
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadBlocking(kSlave, dest, sizeof(dest)));
}


/************************************************************************/
/* Interrupt transfers                                                  */
/************************************************************************/
TEST_F(I2C_Test, WriteInterrupt_NotInit_ReturnsFalse)
{
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteInterrupt(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, WriteInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteInterrupt(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, ReadInterrupt_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadInterrupt(kSlave, dest, sizeof(dest), nullptr));
}

TEST_F(I2C_Test, WriteInterrupt_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeI2C_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteInterrupt(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, ReadInterrupt_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeI2C_SetTransferResult(HAL_ERROR);

    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadInterrupt(kSlave, dest, sizeof(dest), nullptr));
}


/************************************************************************/
/* DMA transfers                                                        */
/************************************************************************/
TEST_F(I2C_Test, WriteDMA_NoTxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDMA(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, WriteDMA_TxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma1_Stream6);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_TRUE(mSubject.WriteDMA(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, WriteDMA_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA tx(DMA::Stream::Dma1_Stream6);
    ASSERT_TRUE(ConfigureDma(tx, DMA::Direction::MemoryToPeripheral));
    ASSERT_TRUE(mSubject.LinkDma(tx));
    FakeI2C_SetTransferResult(HAL_ERROR);

    const uint8_t src[4] = { 1, 2, 3, 4 };
    EXPECT_FALSE(mSubject.WriteDMA(kSlave, src, sizeof(src), nullptr));
}

TEST_F(I2C_Test, ReadDMA_NoRxDmaLinked_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDMA(kSlave, dest, sizeof(dest), nullptr));
}

TEST_F(I2C_Test, ReadDMA_RxDmaLinked_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));

    uint8_t dest[4] = {};
    EXPECT_TRUE(mSubject.ReadDMA(kSlave, dest, sizeof(dest), nullptr));
}

TEST_F(I2C_Test, ReadDMA_HalTransferFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    DMA rx(DMA::Stream::Dma1_Stream5);
    ASSERT_TRUE(ConfigureDma(rx, DMA::Direction::PeripheralToMemory));
    ASSERT_TRUE(mSubject.LinkDma(rx));
    FakeI2C_SetTransferResult(HAL_ERROR);

    uint8_t dest[4] = {};
    EXPECT_FALSE(mSubject.ReadDMA(kSlave, dest, sizeof(dest), nullptr));
}


/************************************************************************/
/* RecoverBus                                                           */
/************************************************************************/
TEST_F(I2C_Test, RecoverBus_NotInit_ReturnsFalse)
{
    PinIdPort scl{ GPIO_PIN_6, GPIOB };
    PinIdPort sda{ GPIO_PIN_7, GPIOB };
    EXPECT_FALSE(mSubject.RecoverBus(scl, sda));
}

TEST_F(I2C_Test, RecoverBus_SdaHeldLow_CompletesAndReInits)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    PinIdPort scl{ GPIO_PIN_6, GPIOB };
    PinIdPort sda{ GPIO_PIN_7, GPIOB };

    // The fake Pin always reads LOW, so SDA never releases -> RecoverBus reports
    // false. The point of the test is that the bit-bang sequence (driven by the
    // fake DWT delay) runs to completion without hanging and the peripheral is
    // re-initialised: a subsequent transfer must still succeed.
    EXPECT_FALSE(mSubject.RecoverBus(scl, sda));

    const uint8_t src[2] = { 0xAB, 0xCD };
    EXPECT_TRUE(mSubject.WriteBlocking(kSlave, src, sizeof(src)));
}


/************************************************************************/
/* IRQ dispatch                                                         */
/************************************************************************/
TEST_F(I2C_Test, EvIRQHandler_AfterInit_DispatchesIntoHal)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_EQ(0, FakeI2C_EvIRQHandlerCallCount());

    I2C1_EV_IRQHandler();

    EXPECT_EQ(1, FakeI2C_EvIRQHandlerCallCount());
}

TEST_F(I2C_Test, ErIRQHandler_AfterInit_DispatchesIntoHal)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_EQ(0, FakeI2C_ErIRQHandlerCallCount());

    I2C1_ER_IRQHandler();

    EXPECT_EQ(1, FakeI2C_ErIRQHandlerCallCount());
}

TEST_F(I2C_Test, EvIRQHandler_AfterSleep_DoesNotDispatch)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Sleep());   // DisconnectCallbacks() drops the slots.

    I2C1_EV_IRQHandler();

    EXPECT_EQ(0, FakeI2C_EvIRQHandlerCallCount());
}


/************************************************************************/
/* Instance coverage (I2C2 / I2C3)                                      */
/************************************************************************/
// Constructing + initialising I2C_2 / I2C_3 exercises the I2C2/I2C3 arms of
// SetInstance, CheckAndEnable/DisablePeripheralClock and GetIRQn (both EV + ER).
TEST_F(I2C_Test, Init_I2c2Instance_ReturnsTrueAndSleeps)
{
    I2C i2c2(I2CInstance::I2C_2);
    EXPECT_TRUE(i2c2.Init(ValidConfig()));
    EXPECT_TRUE(i2c2.Sleep());
}

TEST_F(I2C_Test, Init_I2c3Instance_ReturnsTrueAndSleeps)
{
    I2C i2c3(I2CInstance::I2C_3);
    EXPECT_TRUE(i2c3.Init(ValidConfig()));
    EXPECT_TRUE(i2c3.Sleep());
}

TEST_F(I2C_Test, EvErIRQHandler_I2c2_DispatchesIntoHal)
{
    I2C i2c2(I2CInstance::I2C_2);
    ASSERT_TRUE(i2c2.Init(ValidConfig()));

    I2C2_EV_IRQHandler();
    I2C2_ER_IRQHandler();

    EXPECT_EQ(1, FakeI2C_EvIRQHandlerCallCount());
    EXPECT_EQ(1, FakeI2C_ErIRQHandlerCallCount());
}

TEST_F(I2C_Test, EvErIRQHandler_I2c3_DispatchesIntoHal)
{
    I2C i2c3(I2CInstance::I2C_3);
    ASSERT_TRUE(i2c3.Init(ValidConfig()));

    I2C3_EV_IRQHandler();
    I2C3_ER_IRQHandler();

    EXPECT_EQ(1, FakeI2C_EvIRQHandlerCallCount());
    EXPECT_EQ(1, FakeI2C_ErIRQHandlerCallCount());
}


/************************************************************************/
/* Completion-callback dispatch                                         */
/************************************************************************/
// The async IT paths register a void(bool) user handler the HAL fires on
// completion (MasterTxCplt / MasterRxCplt, success=true) or on error/abort
// (success=false). Iterate every instance so each callback's per-instance
// dispatch arm (I2C1/2/3) is covered.
TEST_F(I2C_Test, WriteInterrupt_TxComplete_EachInstance_FiresHandlerSuccess)
{
    const I2CInstance insts[] = {
        I2CInstance::I2C_1, I2CInstance::I2C_2, I2CInstance::I2C_3
    };
    for (const auto& inst : insts)
    {
        FakeI2C_Reset();
        I2C i2c(inst);
        ASSERT_TRUE(i2c.Init(ValidConfig()));

        bool called = false, ok = false;
        const uint8_t src[4] = { 1, 2, 3, 4 };
        ASSERT_TRUE(i2c.WriteInterrupt(kSlave, src, sizeof(src),
                                       [&](bool s){ called = true; ok = s; }));

        FakeI2C_FireMasterTxCplt();
        EXPECT_TRUE(called);
        EXPECT_TRUE(ok);
    }
}

TEST_F(I2C_Test, ReadInterrupt_RxComplete_EachInstance_FiresHandlerSuccess)
{
    const I2CInstance insts[] = {
        I2CInstance::I2C_1, I2CInstance::I2C_2, I2CInstance::I2C_3
    };
    for (const auto& inst : insts)
    {
        FakeI2C_Reset();
        I2C i2c(inst);
        ASSERT_TRUE(i2c.Init(ValidConfig()));

        bool called = false, ok = false;
        uint8_t dest[4] = {};
        ASSERT_TRUE(i2c.ReadInterrupt(kSlave, dest, sizeof(dest),
                                      [&](bool s){ called = true; ok = s; }));

        FakeI2C_FireMasterRxCplt();
        EXPECT_TRUE(called);
        EXPECT_TRUE(ok);
    }
}

TEST_F(I2C_Test, ErrorCallback_EachInstance_FiresInFlightHandlerFailure)
{
    const I2CInstance insts[] = {
        I2CInstance::I2C_1, I2CInstance::I2C_2, I2CInstance::I2C_3
    };
    for (const auto& inst : insts)
    {
        FakeI2C_Reset();
        I2C i2c(inst);
        ASSERT_TRUE(i2c.Init(ValidConfig()));

        bool called = false, ok = true;
        const uint8_t src[4] = { 1, 2, 3, 4 };
        ASSERT_TRUE(i2c.WriteInterrupt(kSlave, src, sizeof(src),
                                       [&](bool s){ called = true; ok = s; }));

        FakeI2C_FireError();
        EXPECT_TRUE(called);
        EXPECT_FALSE(ok);
    }
}

TEST_F(I2C_Test, AbortCallback_EachInstance_FiresInFlightHandlerFailure)
{
    const I2CInstance insts[] = {
        I2CInstance::I2C_1, I2CInstance::I2C_2, I2CInstance::I2C_3
    };
    for (const auto& inst : insts)
    {
        FakeI2C_Reset();
        I2C i2c(inst);
        ASSERT_TRUE(i2c.Init(ValidConfig()));

        bool called = false, ok = true;
        uint8_t dest[4] = {};
        ASSERT_TRUE(i2c.ReadInterrupt(kSlave, dest, sizeof(dest),
                                      [&](bool s){ called = true; ok = s; }));

        FakeI2C_FireAbort();
        EXPECT_TRUE(called);
        EXPECT_FALSE(ok);
    }
}


} // namespace
