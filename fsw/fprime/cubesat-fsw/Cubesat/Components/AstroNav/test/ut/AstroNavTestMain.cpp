#include "AstroNavTester.hpp"

#include <gtest/gtest.h>

TEST(AstroNav, FirstTick)
{
    Cubesat::AstroNavTester tester;
    tester.testFirstTick();
}

TEST(AstroNav, Advances)
{
    Cubesat::AstroNavTester tester;
    tester.testAdvances();
}

TEST(AstroNav, TelemetryCounts)
{
    Cubesat::AstroNavTester tester;
    tester.testTelemetryCounts();
}
