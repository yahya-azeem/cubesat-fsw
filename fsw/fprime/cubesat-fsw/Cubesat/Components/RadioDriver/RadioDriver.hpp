// ======================================================================
// \title  RadioDriver.hpp
// \brief  PA enable consumer / keying-line stub
// ======================================================================

#ifndef Cubesat_RadioDriver_HPP
#define Cubesat_RadioDriver_HPP

#include "Cubesat/Components/RadioDriver/RadioDriverComponentAc.hpp"

namespace Cubesat {

class RadioDriver final : public RadioDriverComponentBase {
  public:
    explicit RadioDriver(const char* const compName);
    ~RadioDriver() override;

  private:
    void paEnableIn_handler(FwIndexType portNum, U8 enabled) override;

    bool keyed_ = false;
    U32 key_count_ = 0;
};

}  // namespace Cubesat

#endif
