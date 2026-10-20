// ======================================================================
// \title  PowerManagerTester.hpp
// \brief  UT harness for PowerManager
// ======================================================================

#ifndef Cubesat_PowerManagerTester_HPP
#define Cubesat_PowerManagerTester_HPP

#include "Cubesat/Components/PowerManager/PowerManager.hpp"
#include "Cubesat/Components/PowerManager/PowerManagerGTestBase.hpp"

namespace Cubesat {

class PowerManagerTester final : public PowerManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    PowerManagerTester();
    ~PowerManagerTester();

    void testBootSafe();
    void testPassSequence();
    void testFaultLatches();
    void testBatteryGuardBlocksSecondBurst();

  private:
    void connectPorts();
    void initComponents();

    PowerManager component;
};

}  // namespace Cubesat

#endif
