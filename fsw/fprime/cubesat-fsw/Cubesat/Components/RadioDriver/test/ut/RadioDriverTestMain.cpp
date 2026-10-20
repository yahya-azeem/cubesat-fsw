#include "RadioDriverTester.hpp"

#include <gtest/gtest.h>

TEST(RadioDriver, InitiallyInhibited)
{
    Cubesat::RadioDriverTester tester;
    tester.testInitiallyInhibited();
}

TEST(RadioDriver, KeyUp)
{
    Cubesat::RadioDriverTester tester;
    tester.testKeyUp();
}

TEST(RadioDriver, IdempotentKeyUp)
{
    Cubesat::RadioDriverTester tester;
    tester.testIdempotentKeyUp();
}

TEST(RadioDriver, InhibitAndRekey)
{
    Cubesat::RadioDriverTester tester;
    tester.testInhibitAndRekey();
}
