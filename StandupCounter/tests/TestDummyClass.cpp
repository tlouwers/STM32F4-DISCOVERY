#include "gtest/gtest.h"

#include <string>

// Test subject
#include "../target/Src/DummyClass.hpp"

// Mock
// ...


namespace {


// Test fixture for DummyClass
class TEST_DummyClass : public ::testing::Test
{
protected:

    TEST_DummyClass() :
        mSubject()
    {
        // Initialize test matter
    }

    DummyClass mSubject;
};


TEST_F(TEST_DummyClass, simple_multiplication_positive)
{
    constexpr int INP_A = 5;
    constexpr int INP_B = 6;
    constexpr int EXP   = 30;

    EXPECT_EQ(EXP, mSubject.Multiply(INP_A, INP_B));
}

TEST_F(TEST_DummyClass, simple_multiplication_negative)
{
    constexpr int INP_A =   3;
    constexpr int INP_B =  -6;
    constexpr int EXP   = -18;

    EXPECT_EQ(EXP, mSubject.Multiply(INP_A, INP_B));
}


} // namespace
