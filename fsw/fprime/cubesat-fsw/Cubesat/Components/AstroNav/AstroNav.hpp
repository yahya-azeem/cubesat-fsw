// ======================================================================
// \title  AstroNav.hpp
// \brief  CSPICE ephemeris telemetry (passive)
// ======================================================================

#ifndef Cubesat_AstroNav_HPP
#define Cubesat_AstroNav_HPP

#include "Cubesat/Components/AstroNav/AstroNavComponentAc.hpp"
#include "Cubesat/Spice/spice.hpp"

namespace Cubesat {

class AstroNav final : public AstroNavComponentBase {
  public:
    explicit AstroNav(const char* const compName);
    ~AstroNav() override;

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    bool ensure_kernels();
    void publish(const cubesat::spice::State& sat, const cubesat::spice::State& sun, double et);

    cubesat::spice::Kernel leap_seconds_;
    cubesat::spice::Kernel orientation_;
    cubesat::spice::Kernel frames_;
    cubesat::spice::Kernel ephemeris_;

    bool ready_ = false;
    bool failed_ = false;
    double et0_ = 0.0;
    double et_ = 0.0;
};

}  // namespace Cubesat

#endif
