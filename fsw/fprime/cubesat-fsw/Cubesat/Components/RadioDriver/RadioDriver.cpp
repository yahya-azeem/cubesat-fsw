// ======================================================================
// \title  RadioDriver.cpp
// \brief  PA enable consumer / keying-line stub
// ======================================================================

#include "Cubesat/Components/RadioDriver/RadioDriver.hpp"

namespace Cubesat {

RadioDriver::RadioDriver(const char* const compName) : RadioDriverComponentBase(compName) {}

RadioDriver::~RadioDriver() = default;

void RadioDriver::paEnableIn_handler(FwIndexType portNum, U8 enabled)
{
    static_cast<void>(portNum);

    const bool on = enabled != 0;
    this->tlmWrite_PaState(static_cast<U8>(on ? 1 : 0));

    if (on == keyed_) {
        return;  // idempotent: the rate group may repeat the same gate value
    }

    keyed_ = on;
    if (on) {
        ++key_count_;
        this->tlmWrite_KeyCount(key_count_);
        this->log_ACTIVITY_HI_PaKeyed();
    } else {
        this->log_ACTIVITY_LO_PaInhibited();
    }
}

}  // namespace Cubesat
