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
 * \brief   Fake Assert/Expect surface for UnitTestExample native unit tests.
 *          Declares the extern "C" hooks the production header provides.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/UnitTestExample/tests/Fake/utility/Assert
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef ASSERT_H_
#define ASSERT_H_

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdlib.h>


void _expect_log(const char* expr, int line, const char* file);
void _expect_breakpoint();
void _assert_breakpoint();
void _assert_reset(const char* expr, int line, const char* file);


#define EXPECT(expr)  (void)(expr);
#define ASSERT(expr)  (void)(expr);


#ifdef __cplusplus
}
#endif


#endif  // ASSERT_H_
