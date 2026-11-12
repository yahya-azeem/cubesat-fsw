// ======================================================================
// \title  AstroNavTester.cpp
// \brief  UT harness for AstroNav
// ======================================================================

#include "AstroNavTester.hpp"

#include <cmath>

namespace Cubesat {

AstroNavTester::AstroNavTester()
    : AstroNavGTestBase("AstroNavTester", MAX_HISTORY_SIZE), component("AstroNav")
{
    this->initComponents();
    this->connectPorts();
}

AstroNavTester::~AstroNavTester() { this->component.deinit(); }

void AstroNavTester::testFirstTick()
{
    this->invoke_to_schedIn(0, 0);

    ASSERT_EVENTS_KernelsReady_SIZE(1);
    ASSERT_EVENTS_KernelError_SIZE(0);
    ASSERT_EVENTS_EphemerisError_SIZE(0);

    ASSERT_TLM_AltKm_SIZE(1);
    ASSERT_NEAR(this->tlmHistory_AltKm->at(0).arg, 550.0, 1.0);

    ASSERT_TLM_SpeedKmS_SIZE(1);
    ASSERT_NEAR(this->tlmHistory_SpeedKmS->at(0).arg, 7.6, 0.1);

    ASSERT_TLM_PosX_SIZE(1);
    ASSERT_TLM_PosY_SIZE(1);
    ASSERT_TLM_PosZ_SIZE(1);
    ASSERT_TLM_VelX_SIZE(1);
    ASSERT_TLM_VelY_SIZE(1);
    ASSERT_TLM_VelZ_SIZE(1);

    ASSERT_TLM_SunX_SIZE(1);
    ASSERT_TLM_SunY_SIZE(1);
    ASSERT_TLM_SunZ_SIZE(1);
    const double sx = this->tlmHistory_SunX->at(0).arg;
    const double sy = this->tlmHistory_SunY->at(0).arg;
    const double sz = this->tlmHistory_SunZ->at(0).arg;
    ASSERT_NEAR(std::sqrt((sx * sx) + (sy * sy) + (sz * sz)), 1.0, 1.0e-9);

    ASSERT_TLM_LatDeg_SIZE(1);
    ASSERT_TLM_LonDeg_SIZE(1);
    const double lat = this->tlmHistory_LatDeg->at(0).arg;
    const double lon = this->tlmHistory_LonDeg->at(0).arg;
    ASSERT_GE(lat, -90.0);
    ASSERT_LE(lat, 90.0);
    ASSERT_GE(lon, -180.0);
    ASSERT_LE(lon, 180.0);

    ASSERT_TLM_Et_SIZE(1);
    ASSERT_NEAR(this->tlmHistory_Et->at(0).arg, 847713669.183, 1.0);
}

void AstroNavTester::testAdvances()
{
    this->invoke_to_schedIn(0, 0);
    this->invoke_to_schedIn(0, 1);

    ASSERT_TLM_Et_SIZE(2);
    ASSERT_NEAR(this->tlmHistory_Et->at(1).arg - this->tlmHistory_Et->at(0).arg,
                1.0, 1.0e-9);
    ASSERT_TLM_AltKm_SIZE(2);

    // One-second propagation changes the position but not the orbit class.
    ASSERT_NEAR(this->tlmHistory_AltKm->at(1).arg,
                this->tlmHistory_AltKm->at(0).arg, 0.1);
    ASSERT_NEAR(this->tlmHistory_SpeedKmS->at(1).arg,
                this->tlmHistory_SpeedKmS->at(0).arg, 1.0e-3);
}

void AstroNavTester::testTelemetryCounts()
{
    for (FwIndexType i = 0; i < 3; ++i) {
        this->invoke_to_schedIn(0, static_cast<U32>(i));
    }

    ASSERT_TLM_AltKm_SIZE(3);
    ASSERT_TLM_SunX_SIZE(3);
    ASSERT_TLM_LatDeg_SIZE(3);
    ASSERT_TLM_LonDeg_SIZE(3);
    ASSERT_EVENTS_KernelsReady_SIZE(1);  // loaded once, not every tick
    ASSERT_EVENTS_KernelError_SIZE(0);
}

}  // namespace Cubesat
