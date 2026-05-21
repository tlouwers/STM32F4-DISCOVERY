/**
 * \file    Assert.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   No-op assert/expect implementations for FreeRTOSProject native
 *          unit tests (no breakpoints, no resets).
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/FreeRTOSProject/tests/Fake/utility/Assert
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "Assert.h"


void _expect_log(const char* expr, int line, const char* file) { ; }
void _expect_breakpoint() { ; }
void _assert_breakpoint() { ; }
void _assert_reset(const char* expr, int line, const char* file) { ; }
