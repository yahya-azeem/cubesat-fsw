// ======================================================================
// \title  PowerManager.hpp
// \author yahya
// \brief  Passive F´ wrapper around the battery/mode policy
// ======================================================================

#ifndef Cubesat_PowerManager_HPP
#define Cubesat_PowerManager_HPP

#include "Cubesat/Components/PowerManager/PowerManagerComponentAc.hpp"
#include "Cubesat/PowerCore/power.hpp"

namespace Cubesat {

class PowerManager final : public PowerManagerComponentBase {
  public:
    explicit PowerManager(const char* const compName);
    ~PowerManager() override;

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;
    void passScheduled_handler(FwIndexType portNum, U32 context) override;
    void passAcquired_handler(FwIndexType portNum, U32 context) override;
    void downlinkComplete_handler(FwIndexType portNum, U32 context) override;
    void faultIn_handler(FwIndexType portNum, U32 value) override;
    void clearFault_handler(FwIndexType portNum, U32 value) override;

    void CLEAR_FAULT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PASS_SCHEDULED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PASS_ACQUIRED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void DOWNLINK_COMPLETE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    /// Apply one policy event and mirror the result onto ports/telemetry.
    void apply(cubesat::Event event);

    cubesat::Supervisor policy_{};
    cubesat::Battery battery_{2250, 900};  // 2.25 Wh, start at 90 % SOC
};

}  // namespace Cubesat

#endif
