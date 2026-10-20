// ======================================================================
// \title  RadioDriverTester.hpp
// \brief  UT harness for RadioDriver
// ======================================================================

#ifndef Cubesat_RadioDriverTester_HPP
#define Cubesat_RadioDriverTester_HPP

#include "Cubesat/Components/RadioDriver/RadioDriver.hpp"
#include "Cubesat/Components/RadioDriver/RadioDriverGTestBase.hpp"

namespace Cubesat {

class RadioDriverTester final : public RadioDriverGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    RadioDriverTester();
    ~RadioDriverTester();

    void testInitiallyInhibited();
    void testKeyUp();
    void testIdempotentKeyUp();
    void testInhibitAndRekey();

  private:
    void connectPorts();
    void initComponents();

    RadioDriver component;
};

}  // namespace Cubesat

#endif
