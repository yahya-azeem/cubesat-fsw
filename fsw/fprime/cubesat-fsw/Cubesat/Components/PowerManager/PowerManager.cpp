// ======================================================================
// \title  PowerManager.cpp
// \author yahya
// \brief  Passive F´ wrapper around the battery/mode policy
// ======================================================================

#include "Cubesat/Components/PowerManager/PowerManager.hpp"

namespace Cubesat {

namespace {
constexpr U32 kBatteryGuardFault = 0xE0B1U;
constexpr I32 kSbandPowerMw = 13000;
constexpr I32 kSbandBurstSeconds = 300;
constexpr I32 kHarvestPowerMw = 1500;
constexpr U8 kPaOn = 1;
constexpr U8 kPaOff = 0;
}  // namespace

PowerManager::PowerManager(const char* const compName) : PowerManagerComponentBase(compName) {}

PowerManager::~PowerManager() = default;

void PowerManager::apply(cubesat::Event event)
{
    const cubesat::Mode before = policy_.mode();
    const cubesat::Mode now = policy_.step(event);
    if (now == before) {
        return;
    }

    if (now == cubesat::Mode::Downlink) {
        // Never key the PA (or publish Downlink) without the energy for the
        // full burst.
        if (!battery_.discharge(kSbandPowerMw, kSbandBurstSeconds)) {
            static_cast<void>(policy_.step(cubesat::Event::Fault));
            this->tlmWrite_Mode(static_cast<U8>(policy_.mode()));
            this->paEnable_out(0, kPaOff);
            this->log_WARNING_HI_FaultLatched(kBatteryGuardFault);
            return;
        }
        this->tlmWrite_Mode(static_cast<U8>(now));
        this->paEnable_out(0, kPaOn);
        this->log_ACTIVITY_HI_DownlinkEntered();
    } else {
        this->tlmWrite_Mode(static_cast<U8>(now));
        if (before == cubesat::Mode::Downlink) {
            // Inhibit once on the way out; do not spam the port.
            this->paEnable_out(0, kPaOff);
        }
    }
}

void PowerManager::schedIn_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);

    // 1 Hz rate group: one second of harvest per tick.
    if (policy_.mode() == cubesat::Mode::Charge) {
        static_cast<void>(battery_.charge(kHarvestPowerMw, 1));
    }

    this->tlmWrite_BatteryMwh(battery_.stored_mwh());
    this->tlmWrite_SocPermille(static_cast<U16>(battery_.soc_permille()));
}

void PowerManager::passScheduled_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);
    apply(cubesat::Event::PassScheduled);
}

void PowerManager::passAcquired_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);
    apply(cubesat::Event::PassAcquired);
}

void PowerManager::downlinkComplete_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);
    apply(cubesat::Event::DownlinkComplete);
}

void PowerManager::faultIn_handler(FwIndexType portNum, U32 value)
{
    static_cast<void>(portNum);
    static_cast<void>(policy_.step(cubesat::Event::Fault));
    this->paEnable_out(0, kPaOff);
    this->tlmWrite_Mode(static_cast<U8>(policy_.mode()));
    this->log_WARNING_HI_FaultLatched(value);
}

void PowerManager::clearFault_handler(FwIndexType portNum, U32 value)
{
    static_cast<void>(portNum);
    static_cast<void>(value);
    apply(cubesat::Event::FaultCleared);
}

// ---- ground commands -------------------------------------------------------

void PowerManager::CLEAR_FAULT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
{
    apply(cubesat::Event::FaultCleared);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PowerManager::PASS_SCHEDULED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
{
    apply(cubesat::Event::PassScheduled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PowerManager::PASS_ACQUIRED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
{
    apply(cubesat::Event::PassAcquired);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PowerManager::DOWNLINK_COMPLETE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
{
    apply(cubesat::Event::DownlinkComplete);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Cubesat
