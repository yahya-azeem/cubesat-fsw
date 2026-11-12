// ======================================================================
// \title  AstroNavTester.hpp
// \brief  UT harness for AstroNav
// ======================================================================

#ifndef Cubesat_AstroNavTester_HPP
#define Cubesat_AstroNavTester_HPP

#include "Cubesat/Components/AstroNav/AstroNav.hpp"
#include "Cubesat/Components/AstroNav/AstroNavGTestBase.hpp"

namespace Cubesat {

class AstroNavTester final : public AstroNavGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    AstroNavTester();
    ~AstroNavTester();

    void testFirstTick();
    void testAdvances();
    void testTelemetryCounts();

  private:
    void connectPorts();
    void initComponents();

    AstroNav component;
};

}  // namespace Cubesat

#endif
