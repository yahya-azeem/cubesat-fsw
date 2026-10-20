// ======================================================================
// \title  DataStore.hpp
// \brief  LittleFS housekeeping recorder (passive)
// ======================================================================

#ifndef Cubesat_DataStore_HPP
#define Cubesat_DataStore_HPP

#include "Cubesat/Components/DataStore/DataStoreComponentAc.hpp"
#include "Cubesat/Storage/file_store.hpp"

namespace Cubesat {

class DataStore final : public DataStoreComponentBase {
  public:
    explicit DataStore(const char* const compName);
    ~DataStore() override;

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    cubesat::RamBlock device_{};
    cubesat::FileStore store_{device_};
    bool mounted_ = false;
    U32 records_ = 0;
};

}  // namespace Cubesat

#endif
