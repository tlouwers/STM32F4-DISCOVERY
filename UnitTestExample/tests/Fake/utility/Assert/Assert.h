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
