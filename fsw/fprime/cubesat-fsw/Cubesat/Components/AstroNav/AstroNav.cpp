// ======================================================================
// \title  AstroNav.cpp
// \brief  CSPICE ephemeris telemetry (passive)
// ======================================================================

#include "Cubesat/Components/AstroNav/AstroNav.hpp"

#include "kernel_dir.hpp"

#include <array>
#include <cmath>
#include <cstdio>

namespace sp = cubesat::spice;

namespace Cubesat {

namespace {

constexpr double kEarthRadiusKm = 6378.137;
constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kCoverageSeconds = 86399.0;  // demo SPK is one day long

bool join_path(std::array<char, sp::kMaxPath>& out, const char* name) noexcept
{
    const int written = std::snprintf(out.data(), out.size(), "%s/%s",
                                      CUBESAT_KERNEL_DIR, name);
    return written > 0 && static_cast<std::size_t>(written) < out.size();
}

double norm3(const std::array<double, 3>& v) noexcept
{
    return std::sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
}

double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

bool load_kernel(sp::Kernel& kernel, const char* name, I32& code) noexcept
{
    std::array<char, sp::kMaxPath> path{};
    if (!join_path(path, name)) {
        code = static_cast<I32>(sp::Code::InvalidArgument);
        return false;
    }
    const auto result = kernel.load(path.data());
    if (!result) {
        code = static_cast<I32>(result.error().code);
        return false;
    }
    return true;
}

}  // namespace

AstroNav::AstroNav(const char* const compName) : AstroNavComponentBase(compName) {}

AstroNav::~AstroNav() = default;

bool AstroNav::ensure_kernels()
{
    I32 code = static_cast<I32>(sp::Code::KernelLoad);

    if (!load_kernel(leap_seconds_, "naif0012.tls", code) ||
        !load_kernel(orientation_, "pck00011.tpc", code) ||
        !load_kernel(frames_, "cubesat.tf", code) ||
        !load_kernel(ephemeris_, "cubesat_demo.bsp", code)) {
        this->log_WARNING_HI_KernelError(code);
        failed_ = true;
        return false;
    }

    const auto et0 = sp::utc_to_et("2026-11-12T00:00:00");
    if (!et0) {
        this->log_WARNING_HI_KernelError(static_cast<I32>(et0.error().code));
        failed_ = true;
        return false;
    }

    et0_ = *et0;
    et_ = et0_;
    ready_ = true;
    this->log_ACTIVITY_HI_KernelsReady();
    return true;
}

void AstroNav::publish(const sp::State& sat, const sp::State& sun, double et)
{
    const double range = norm3(sat.position_km);
    const double speed = norm3(sat.velocity_km_s);
    const double sun_range = norm3(sun.position_km);

    double sun_x = 0.0;
    double sun_y = 0.0;
    double sun_z = 0.0;
    if (sun_range > 0.0 && std::isfinite(sun_range)) {
        sun_x = sun.position_km[0] / sun_range;
        sun_y = sun.position_km[1] / sun_range;
        sun_z = sun.position_km[2] / sun_range;
    }

    double lat = 0.0;
    double lon = 0.0;
    const auto to_ecef = sp::frame_transform("J2000", "IAU_EARTH", et);
    if (to_ecef && range > 0.0) {
        const auto& m = to_ecef->m;
        const double x = dot3(m[0], sat.position_km);
        const double y = dot3(m[1], sat.position_km);
        const double z = dot3(m[2], sat.position_km);
        lat = std::asin(z / range) * kRadToDeg;
        lon = std::atan2(y, x) * kRadToDeg;
    }

    this->tlmWrite_Et(et);
    this->tlmWrite_AltKm(range - kEarthRadiusKm);
    this->tlmWrite_SpeedKmS(speed);
    this->tlmWrite_PosX(sat.position_km[0]);
    this->tlmWrite_PosY(sat.position_km[1]);
    this->tlmWrite_PosZ(sat.position_km[2]);
    this->tlmWrite_VelX(sat.velocity_km_s[0]);
    this->tlmWrite_VelY(sat.velocity_km_s[1]);
    this->tlmWrite_VelZ(sat.velocity_km_s[2]);
    this->tlmWrite_SunX(sun_x);
    this->tlmWrite_SunY(sun_y);
    this->tlmWrite_SunZ(sun_z);
    this->tlmWrite_LatDeg(lat);
    this->tlmWrite_LonDeg(lon);
}

void AstroNav::schedIn_handler(FwIndexType portNum, U32 context)
{
    static_cast<void>(portNum);
    static_cast<void>(context);

    if (failed_) {
        return;
    }
    if (!ready_ && !ensure_kernels()) {
        return;
    }

    const auto sat = sp::state("CUBESAT", et_, "J2000", "NONE", "EARTH");
    if (!sat) {
        this->log_WARNING_LO_EphemerisError(static_cast<I32>(sat.error().code));
        et_ += 1.0;
        if (et_ > et0_ + kCoverageSeconds) {
            et_ = et0_;  // demo kernel loops daily
        }
        return;
    }

    const auto sun = sp::state("SUN", et_, "J2000", "NONE", "EARTH");
    if (!sun) {
        this->log_WARNING_LO_EphemerisError(static_cast<I32>(sun.error().code));
    }

    publish(*sat, sun ? *sun : sp::State{}, et_);

    et_ += 1.0;
    if (et_ > et0_ + kCoverageSeconds) {
        et_ = et0_;
    }
}

}  // namespace Cubesat
