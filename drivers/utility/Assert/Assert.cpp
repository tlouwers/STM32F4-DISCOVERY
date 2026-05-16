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
 * \brief   Custom implementation of ASSERT() and EXPECT().
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/utility/Assert
 *
 * \details Implemented with 'C' header and 'C++' implementation to
 *          be usable from both languages. Intended use is to provide
 *          more fine-grained assert logic.
 *
 * \note    Only use the macros, not the functions - these are intended
 *          to only be used by these macros.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Assert.h"
#include <cstdio>
//#include "components/Logging.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint8_t MAX_LOG_LINE_SIZE = 128;   ///< In bytes, including closing '\0'.


/************************************************************************/
/* Functions                                                            */
/************************************************************************/
/**
 * \brief   Handler for Assert_ExpectLog().
 * \details Add log entry of the EXPECT() occurrence with some details.
 * \param   expr    The failing expression, as string.
 * \param   line    The line number at which the expect occurred.
 * \param   file    The file in which the expect occurred.
 */
void Assert_ExpectLog(const char* expr, int line, const char* file)
{
    char messBuff[MAX_LOG_LINE_SIZE] = {};
    if (snprintf(messBuff, MAX_LOG_LINE_SIZE, "EXPECT: [%s], line: [%d], file: [%s]", expr, line, file) > 0)
    {
        // ToDo: enable when real logger is implemented...
//        Logging::Static_Log(LogLevel::ERROR, messBuff);
    }
}

/**
 * \brief   Handler for Assert_ExpectBreakpoint().
 * \details If a debugger is attached, BKPT halts execution at this
 *          instruction; if not, the CPU takes the HardFault exception and
 *          execution stops in the default fault handler.
 */
void Assert_ExpectBreakpoint(void)
{
    __asm volatile("BKPT #01");     // Break into the debugger
}

/**
 * \brief   Handler for Assert_Breakpoint().
 * \details If a debugger is attached, BKPT halts execution at this
 *          instruction; if not, the CPU takes the HardFault exception and
 *          execution stops in the default fault handler.
 */
void Assert_Breakpoint(void)
{
    __asm volatile("BKPT #01");     // Break into the debugger
}

/**
 * \brief   Handler for Assert_Reset().
 * \details Save state, reset, log after reboot.
 * \param   expr    The failing expression, as string.
 * \param   line    The line number at which the assert occurred.
 * \param   file    The file in which the assert occurred.
 */
__attribute__((noreturn)) void Assert_Reset(const char* expr, int line, const char* file)
{
    // Preserve state, log on bootup, ...

    (void)(expr);                   // Hide compiler warnings
    (void)(line);
    (void)(file);

    HAL_NVIC_SystemReset();         // Perform reset of the microcontroller.

    for (;;) { }                    // Defensive: HAL_NVIC_SystemReset never returns,
                                    // but [[noreturn]] needs a non-returning body too.
}
