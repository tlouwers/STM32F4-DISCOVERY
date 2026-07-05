/**
 * \file    TestBootloaderEntry.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Unit tests for the BootloaderEntry module: magic-register check,
 *          jump/clear ordering, guard conditions, and the factory-reset
 *          trigger sequence. Hardware is substituted by MockBootloaderHal.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "bootloader/BootloaderEntry.hpp"
#include "MockBootloaderHal.hpp"


using ::testing::Return;
using ::testing::InSequence;


/************************************************************************/
/* Test fixture                                                         */
/************************************************************************/
class BootloaderEntry_Test : public ::testing::Test
{
protected:
    MockBootloaderHal mHal;
    BootloaderEntry   mSubject { mHal };
};


/************************************************************************/
/* CheckAndEnterBootloader                                              */
/************************************************************************/
TEST_F(BootloaderEntry_Test, CheckAndEnterBootloader_MagicPresent_ClearsMagicThenJumps)
{
    InSequence seq;
    EXPECT_CALL(mHal, ReadMagic())
        .WillOnce(Return(BootloaderEntry::kFactoryResetMagic));
    EXPECT_CALL(mHal, WriteMagic(0u));            // cleared before the jump
    EXPECT_CALL(mHal, JumpToSystemMemory());

    EXPECT_TRUE(mSubject.CheckAndEnterBootloader());
}

TEST_F(BootloaderEntry_Test, CheckAndEnterBootloader_MagicAbsent_BootsNormally)
{
    EXPECT_CALL(mHal, ReadMagic())
        .WillOnce(Return(0u));
    EXPECT_CALL(mHal, WriteMagic(::testing::_)).Times(0);
    EXPECT_CALL(mHal, JumpToSystemMemory()).Times(0);

    EXPECT_FALSE(mSubject.CheckAndEnterBootloader());
}

TEST_F(BootloaderEntry_Test, CheckAndEnterBootloader_WrongMagic_BootsNormally)
{
    EXPECT_CALL(mHal, ReadMagic())
        .WillOnce(Return(0x12345678u));
    EXPECT_CALL(mHal, JumpToSystemMemory()).Times(0);

    EXPECT_FALSE(mSubject.CheckAndEnterBootloader());
}


/************************************************************************/
/* TriggerFactoryReset                                                  */
/************************************************************************/
TEST_F(BootloaderEntry_Test, TriggerFactoryReset_WritesMagicThenResets)
{
    InSequence seq;
    EXPECT_CALL(mHal, WriteMagic(BootloaderEntry::kFactoryResetMagic));
    EXPECT_CALL(mHal, SystemReset());

    mSubject.TriggerFactoryReset();
}

TEST_F(BootloaderEntry_Test, TriggerFactoryReset_UsesExpectedMagicValue)
{
    EXPECT_EQ(0xDEADBEEFu, BootloaderEntry::kFactoryResetMagic);
}
