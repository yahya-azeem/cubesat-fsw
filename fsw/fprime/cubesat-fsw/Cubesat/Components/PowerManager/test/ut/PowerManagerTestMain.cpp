#include "PowerManagerTester.hpp"

#include <gtest/gtest.h>

TEST(PowerManager, BootSafe)
{
    Cubesat::PowerManagerTester tester;
    tester.testBootSafe();
}

TEST(PowerManager, PassSequence)
{
    Cubesat::PowerManagerTester tester;
    tester.testPassSequence();
}

TEST(PowerManager, FaultLatches)
{
    Cubesat::PowerManagerTester tester;
    tester.testFaultLatches();
}

TEST(PowerManager, BatteryGuardBlocksSecondBurst)
{
    Cubesat::PowerManagerTester tester;
    tester.testBatteryGuardBlocksSecondBurst();
}
