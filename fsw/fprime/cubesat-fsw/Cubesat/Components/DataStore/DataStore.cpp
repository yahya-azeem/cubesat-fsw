// ======================================================================
// \title  DataStore.cpp
// \brief  LittleFS housekeeping recorder (passive)
// ======================================================================

#include "Cubesat/Components/DataStore/DataStore.hpp"

#include <array>
#include <cstdint>

namespace Cubesat {

namespace {
constexpr const char* kHousekeepingPath = "hk.log";
constexpr std::size_t kRecordBytes = 8;
}  // namespace

DataStore::DataStore(const char* const compName) : DataStoreComponentBase(compName) {}

DataStore::~DataStore() = default;

void DataStore::schedIn_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);

    if (!mounted_) {
        mounted_ = store_.mount();
        this->tlmWrite_Mounted(static_cast<U8>(mounted_ ? 1 : 0));
        if (!mounted_) {
            return;
        }
        this->log_ACTIVITY_HI_StorageReady();
    }

    std::array<std::uint8_t, kRecordBytes> record{};
    for (std::size_t i = 0; i < record.size(); ++i) {
        record[i] = static_cast<std::uint8_t>((records_ >> (8U * i)) & 0xFFU);
    }

    const lfs_ssize_t written =
        store_.append(kHousekeepingPath, record.data(), static_cast<lfs_size_t>(record.size()));
    if (written < 0) {
        this->log_WARNING_HI_WriteFailed(static_cast<I32>(written));
        return;
    }

    ++records_;
    this->tlmWrite_Records(records_);
    this->tlmWrite_LastBytes(static_cast<U32>(written));
}

}  // namespace Cubesat
