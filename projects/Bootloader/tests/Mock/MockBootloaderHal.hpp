/**
 * \file    MockBootloaderHal.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   MockBootloaderHal
 *
 * \brief   GMock implementation of the IBootloaderHal interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef MOCK_BOOTLOADER_HAL_HPP_
#define MOCK_BOOTLOADER_HAL_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "bootloader/IBootloaderHal.hpp"
#include "gmock/gmock.h"
#include <cstdint>


class MockBootloaderHal final : public IBootloaderHal
{
public:
    MOCK_CONST_METHOD0(ReadMagic, uint32_t());
    MOCK_METHOD1(WriteMagic, void(uint32_t value));
    MOCK_METHOD0(SystemReset, void());
    MOCK_METHOD0(JumpToSystemMemory, void());
};


#endif  // MOCK_BOOTLOADER_HAL_HPP_
