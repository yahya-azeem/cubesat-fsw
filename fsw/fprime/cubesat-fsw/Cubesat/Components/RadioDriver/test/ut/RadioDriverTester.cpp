// ======================================================================
// \title  RadioDriverTester.cpp
// \brief  UT harness for RadioDriver
// ======================================================================

#include "RadioDriverTester.hpp"

namespace Cubesat {

RadioDriverTester::RadioDriverTester()
    : RadioDriverGTestBase("RadioDriverTester", MAX_HISTORY_SIZE), component("RadioDriver")
{
    this->initComponents();
    this->connectPorts();
}

RadioDriverTester::~RadioDriverTester() { this->component.deinit(); }

void RadioDriverTester::testInitiallyInhibited()
{
    this->invoke_to_paEnableIn(0, 0);

    ASSERT_TLM_PaState_SIZE(1);
    ASSERT_TLM_PaState(0, 0);
    ASSERT_TLM_KeyCount_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void RadioDriverTester::testKeyUp()
{
    this->invoke_to_paEnableIn(0, 1);

    ASSERT_TLM_PaState_SIZE(1);
    ASSERT_TLM_PaState(0, 1);
    ASSERT_TLM_KeyCount_SIZE(1);
    ASSERT_TLM_KeyCount(0, 1);
    ASSERT_EVENTS_PaKeyed_SIZE(1);
    ASSERT_EVENTS_PaInhibited_SIZE(0);
}

void RadioDriverTester::testIdempotentKeyUp()
{
    this->invoke_to_paEnableIn(0, 1);
    this->invoke_to_paEnableIn(0, 1);
    this->invoke_to_paEnableIn(0, 1);

    // The rate group may repeat the gate value; key-ups must not double count.
    ASSERT_TLM_PaState_SIZE(3);
    ASSERT_TLM_KeyCount_SIZE(1);
    ASSERT_TLM_KeyCount(0, 1);
    ASSERT_EVENTS_PaKeyed_SIZE(1);
}

void RadioDriverTester::testInhibitAndRekey()
{
    this->invoke_to_paEnableIn(0, 1);
    this->invoke_to_paEnableIn(0, 0);
    this->invoke_to_paEnableIn(0, 1);

    ASSERT_TLM_PaState_SIZE(3);
    ASSERT_TLM_PaState(0, 1);
    ASSERT_TLM_PaState(1, 0);
    ASSERT_TLM_PaState(2, 1);

    ASSERT_TLM_KeyCount_SIZE(2);
    ASSERT_TLM_KeyCount(0, 1);
    ASSERT_TLM_KeyCount(1, 2);

    ASSERT_EVENTS_PaKeyed_SIZE(2);
    ASSERT_EVENTS_PaInhibited_SIZE(1);
}

}  // namespace Cubesat
