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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/utility/Assert
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
 * \brief   Handler for expect_log().
 * \details Add log entry of the EXPECT() occurance with some details.
 * \param   expr    The failing expression, as string.
 * \param   line    The line number at which the expect occurred.
 * \param   file    The file in which the expect occurred.
 */
void _expect_log(const char* expr, int line, const char* file)
{
    char messBuff[MAX_LOG_LINE_SIZE] = {};
    if (snprintf(messBuff, MAX_LOG_LINE_SIZE, "EXPECT: [%s], line: [%d], file: [%s]", expr, line, file))
    {
        // ToDo: enable when real logger is implemented...
//        Logging::Static_Log(LogLevel::ERROR, messBuff);
    }
}

/**
 * \brief   Handler for expect_breakpoint().
 * \details Enter breakpoint when hit, loop forever if not actively debugging.
 */
void _expect_breakpoint()
{
    __asm volatile("BKPT #01");     // Break into the debugger
}

/**
 * \brief   Handler for assert_breakpoint().
 * \details Enter breakpoint when hit, loop forever if not actively debugging.
 */
void _assert_breakpoint()
{
    __asm volatile("BKPT #01");     // Break into the debugger
}

/**
 * \brief   Handler for the assert_reset.
 * \details Save state, reset, log after reboot.
 * \param   expr    The failing expression, as string.
 * \param   line    The line number at which the assert occurred.
 * \param   file    The file in which the assert occurred.
 */
void _assert_reset(const char* expr, int line, const char* file)
{
    // Preserve state, log on bootup, ...

    HAL_NVIC_SystemReset();         // Perform reset of the microcontroller.

    (void)(expr);                   // Hide compiler warnings
    (void)(line);
    (void)(file);
}
