// ======================================================================
// \title  DataStoreTester.hpp
// \brief  UT harness for DataStore
// ======================================================================

#ifndef Cubesat_DataStoreTester_HPP
#define Cubesat_DataStoreTester_HPP

#include "Cubesat/Components/DataStore/DataStore.hpp"
#include "Cubesat/Components/DataStore/DataStoreGTestBase.hpp"

namespace Cubesat {

class DataStoreTester final : public DataStoreGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    DataStoreTester();
    ~DataStoreTester();

    void testFirstTickMounts();
    void testSecondTickAppends();
    void testRecordsAreMonotonic();

  private:
    void connectPorts();
    void initComponents();

    DataStore component;
};

}  // namespace Cubesat

#endif
