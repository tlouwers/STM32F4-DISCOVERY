/**
 * \file    Assert.h
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

#ifndef ASSERT_H_
#define ASSERT_H_


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "config.h"
#include <stdlib.h>


/************************************************************************/
/* Function declarations                                                */
/************************************************************************/
void _expect_log(const char* expr, int line, const char* file);
void _expect_breakpoint(void);
void _assert_breakpoint(void);
__attribute__((noreturn)) void _assert_reset(const char* expr, int line, const char* file);


/************************************************************************/
/* Macros                                                               */
/************************************************************************/
/**
 * \brief   An EXPECT() is used if you think the device can continue execution
 *          a little while longer. For instance: nothing will break inside the
 *          method where EXPECT() is placed. Checks around buffers if there is
 *          still room available could be of this type.
 * \note    The do/while(0) wrapper makes the macro a single statement so it
 *          composes correctly with `if`/`else` (no dangling-else hazard) and
 *          requires the caller's trailing semicolon.
 */
#if (EXPECT_MODE == HANDLE_BY_IGNORING)
#  define EXPECT(expr)  do { (void)(expr); } while (0)

#elif (EXPECT_MODE == HANDLE_BY_LOGGING)
#  define EXPECT(expr)  do { if (!(expr)) { _expect_log(#expr, __LINE__, __FILE__); } } while (0)

#elif (EXPECT_MODE == HANDLE_BY_BREAKPOINT)
#  define EXPECT(expr)  do { if (!(expr)) { _expect_breakpoint(); } } while (0)

#else
#  error EXPECT_MODE has unsupported value
#endif


/**
 * \brief   An ASSERT() is used if at that point where the device cannot recover.
 *          For instance with a value that is absolutely not allowed and if used
 *          will stop the device from working.
 */
#if (ASSERT_MODE == HANDLE_BY_IGNORING)
#  define ASSERT(expr)  do { (void)(expr); } while (0)

#elif (ASSERT_MODE == HANDLE_BY_BREAKPOINT)
#  define ASSERT(expr)  do { if (!(expr)) { _assert_breakpoint(); } } while (0)

#elif (ASSERT_MODE == HANDLE_BY_RESETTING)
#  define ASSERT(expr)  do { if (!(expr)) { _assert_reset(#expr, __LINE__, __FILE__); } } while (0)

#else
#  error ASSERT_MODE has an unsupported value
#endif


#ifdef __cplusplus
}
#endif


#endif  // ASSERT_H_
