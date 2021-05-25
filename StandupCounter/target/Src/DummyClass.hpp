#ifndef DUMMYCLASS_HPP_
#define DUMMYCLASS_HPP_

/******************************************************************************
 * Includes                                                                   *
 *****************************************************************************/
#include <cstdint>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Dummy class.
 */
class DummyClass
{
public:
    DummyClass() {}
    virtual ~DummyClass() {}

    int Multiply(int a, int b);
};


#endif  // DUMMYCLASS_HPP_
