// ======================================================================
// \title  PowerManagerTester.cpp
// \brief  UT harness for PowerManager
// ======================================================================

#include "PowerManagerTester.hpp"

namespace Cubesat {

PowerManagerTester::PowerManagerTester()
    : PowerManagerGTestBase("PowerManagerTester", MAX_HISTORY_SIZE), component("PowerManager")
{
    this->initComponents();
    this->connectPorts();
}

PowerManagerTester::~PowerManagerTester() { this->component.deinit(); }

void PowerManagerTester::testBootSafe()
{
    this->invoke_to_schedIn(0, 0);
    this->invoke_to_passAcquired(0, 0);  // no scheduled pass: must be ignored

    ASSERT_TLM_Mode_SIZE(0);
    ASSERT_TLM_SocPermille_SIZE(1);
    ASSERT_TLM_SocPermille(0, 900);
    ASSERT_TLM_BatteryMwh_SIZE(1);
    ASSERT_TLM_BatteryMwh(0, 2025);
    ASSERT_from_paEnable_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void PowerManagerTester::testPassSequence()
{
    this->invoke_to_clearFault(0, 0);
    this->invoke_to_schedIn(0, 0);
    this->invoke_to_passScheduled(0, 0);
    this->invoke_to_passAcquired(0, 0);
    this->invoke_to_downlinkComplete(0, 0);

    ASSERT_EVENTS_DownlinkEntered_SIZE(1);
    ASSERT_EVENTS_FaultLatched_SIZE(0);

    ASSERT_TLM_Mode_SIZE(4);
    ASSERT_TLM_Mode(0, 1);  // Charge
    ASSERT_TLM_Mode(1, 2);  // AwaitPass
    ASSERT_TLM_Mode(2, 3);  // Downlink
    ASSERT_TLM_Mode(3, 1);  // Charge

    ASSERT_from_paEnable_SIZE(2);
    ASSERT_from_paEnable(0, static_cast<U8>(1));  // keyed on AOS
    ASSERT_from_paEnable(1, static_cast<U8>(0));  // inhibited on LOS
}

void PowerManagerTester::testFaultLatches()
{
    this->invoke_to_clearFault(0, 0);
    this->invoke_to_faultIn(0, 0xBEEFU);

    ASSERT_EVENTS_FaultLatched_SIZE(1);
    ASSERT_EVENTS_FaultLatched(0, 0xBEEFU);

    ASSERT_TLM_Mode_SIZE(2);
    ASSERT_TLM_Mode(0, 1);  // Charge after clear
    ASSERT_TLM_Mode(1, 0);  // Safe after fault
    ASSERT_from_paEnable_SIZE(1);
    ASSERT_from_paEnable(0, static_cast<U8>(0));  // fail-closed: PA inhibited

    // Fault is sticky: pass events are ignored until cleared again.
    this->invoke_to_passScheduled(0, 0);
    this->invoke_to_passAcquired(0, 0);
    ASSERT_TLM_Mode_SIZE(2);
    ASSERT_from_paEnable_SIZE(1);
}

void PowerManagerTester::testBatteryGuardBlocksSecondBurst()
{
    this->invoke_to_clearFault(0, 0);

    // First burst: 2.025 Wh stored, 1.083 Wh spent.
    this->invoke_to_schedIn(0, 0);
    this->invoke_to_passScheduled(0, 0);
    this->invoke_to_passAcquired(0, 0);
    this->invoke_to_downlinkComplete(0, 0);
    ASSERT_EVENTS_DownlinkEntered_SIZE(1);

    // Second burst without a recharge would cross the 20 % reserve.
    this->invoke_to_passScheduled(0, 0);
    this->invoke_to_passAcquired(0, 0);

    ASSERT_EVENTS_FaultLatched_SIZE(1);
    ASSERT_TLM_Mode_SIZE(6);
    ASSERT_TLM_Mode(0, 1);  // Charge
    ASSERT_TLM_Mode(1, 2);  // AwaitPass
    ASSERT_TLM_Mode(2, 3);  // Downlink
    ASSERT_TLM_Mode(3, 1);  // Charge
    ASSERT_TLM_Mode(4, 2);  // AwaitPass
    ASSERT_TLM_Mode(5, 0);  // Safe

    // PA keyed once, inhibited on completion, and stayed inhibited.
    ASSERT_from_paEnable_SIZE(3);
    ASSERT_from_paEnable(0, static_cast<U8>(1));
    ASSERT_from_paEnable(1, static_cast<U8>(0));
    ASSERT_from_paEnable(2, static_cast<U8>(0));
}

}  // namespace Cubesat
