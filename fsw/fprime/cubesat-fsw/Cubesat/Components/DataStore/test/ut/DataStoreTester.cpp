// ======================================================================
// \title  DataStoreTester.cpp
// \brief  UT harness for DataStore
// ======================================================================

#include "DataStoreTester.hpp"

namespace Cubesat {

DataStoreTester::DataStoreTester()
    : DataStoreGTestBase("DataStoreTester", MAX_HISTORY_SIZE), component("DataStore")
{
    this->initComponents();
    this->connectPorts();
}

DataStoreTester::~DataStoreTester() { this->component.deinit(); }

void DataStoreTester::testFirstTickMounts()
{
    this->invoke_to_schedIn(0, 0);

    ASSERT_EVENTS_StorageReady_SIZE(1);
    ASSERT_EVENTS_WriteFailed_SIZE(0);

    ASSERT_TLM_Mounted_SIZE(1);
    ASSERT_TLM_Mounted(0, 1);
    ASSERT_TLM_Records_SIZE(1);
    ASSERT_TLM_Records(0, 1);
    ASSERT_TLM_LastBytes_SIZE(1);
    ASSERT_TLM_LastBytes(0, 8);
}

void DataStoreTester::testSecondTickAppends()
{
    this->invoke_to_schedIn(0, 0);
    this->invoke_to_schedIn(0, 0);

    // Mount happens once; the record counter advances every tick.
    ASSERT_EVENTS_StorageReady_SIZE(1);
    ASSERT_TLM_Mounted_SIZE(1);
    ASSERT_TLM_Records_SIZE(2);
    ASSERT_TLM_Records(0, 1);
    ASSERT_TLM_Records(1, 2);
    ASSERT_TLM_LastBytes_SIZE(2);
}

void DataStoreTester::testRecordsAreMonotonic()
{
    for (FwIndexType i = 0; i < 5; ++i) {
        this->invoke_to_schedIn(0, static_cast<U32>(i));
    }

    ASSERT_TLM_Records_SIZE(5);
    for (FwIndexType i = 0; i < 5; ++i) {
        ASSERT_TLM_Records(i, static_cast<U32>(i + 1));
    }
    ASSERT_EVENTS_WriteFailed_SIZE(0);
}

}  // namespace Cubesat
