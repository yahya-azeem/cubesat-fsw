#include "DataStoreTester.hpp"

#include <gtest/gtest.h>

TEST(DataStore, FirstTickMounts)
{
    Cubesat::DataStoreTester tester;
    tester.testFirstTickMounts();
}

TEST(DataStore, SecondTickAppends)
{
    Cubesat::DataStoreTester tester;
    tester.testSecondTickAppends();
}

TEST(DataStore, RecordsAreMonotonic)
{
    Cubesat::DataStoreTester tester;
    tester.testRecordsAreMonotonic();
}
