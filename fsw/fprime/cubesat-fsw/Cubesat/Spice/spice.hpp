// Bounded, fail-closed C++23 wrapper around CSPICE.
//
// CSPICE is a C library with process-global error state and fixed-size
// buffers. This wrapper:
//   * copies caller strings into fixed buffers and rejects overlong input,
//     embedded NULs and non-finite numbers,
//   * switches CSPICE to RETURN error mode (no abort, no stderr spam),
//   * returns std::expected<T, Error> instead of relying on side effects,
//   * validates states (finite) and frame matrices (orthonormal, det = +1),
//   * owns kernels with RAII and is move-only.
//
// No exceptions, no RTTI, no heap. CSPICE is not thread-safe: call from one
// thread and finish kernel loading before the scheduler starts.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace cubesat::spice {

inline constexpr std::size_t kMaxPath = 512;
inline constexpr std::size_t kMaxName = 64;
inline constexpr std::size_t kMaxTime = 64;
inline constexpr std::size_t kMaxUtc = 40;
inline constexpr std::size_t kMaxError = 184;

/// Fixed-capacity, always-NUL-terminated text.
template <std::size_t N>
struct Text {
    std::array<char, N> data{};
    std::size_t length = 0;

    [[nodiscard]] constexpr const char* c_str() const noexcept { return data.data(); }
    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {data.data(), length};
    }
};

enum class Code : std::uint8_t {
    InvalidArgument = 1,
    KernelLoad,
    TimeConversion,
    Ephemeris,
    Frame,
    NonFinite,
    SpiceFailure,
};

struct Error {
    Code code = Code::SpiceFailure;
    Text<kMaxError> message{};
};

template <typename T>
using Result = std::expected<T, Error>;

enum class UtcFormat : std::uint8_t { Isoc, Isod, Cals, Jansc };

struct State {
    std::array<double, 3> position_km{};
    std::array<double, 3> velocity_km_s{};
    double light_time_s = 0.0;
};

struct Matrix3 {
    std::array<std::array<double, 3>, 3> m{};
};

/// Installs CSPICE error handling. Idempotent; called by every entry point.
void initialize() noexcept;

/// RAII kernel handle: furnishes on load, unloads on destruction.
class Kernel {
  public:
    Kernel() noexcept = default;
    ~Kernel();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;
    Kernel(Kernel&& other) noexcept;
    Kernel& operator=(Kernel&& other) noexcept;

    [[nodiscard]] Result<void> load(std::string_view path) noexcept;
    [[nodiscard]] Result<void> unload() noexcept;
    [[nodiscard]] bool loaded() const noexcept { return loaded_; }

  private:
    std::array<char, kMaxPath> path_{};
    bool loaded_ = false;
};

/// UTC string -> ephemeris seconds past J2000. Needs a leapseconds kernel.
[[nodiscard]] Result<double> utc_to_et(std::string_view utc) noexcept;

/// ET -> UTC. Precision is limited to 0..9; anything else is rejected.
[[nodiscard]] Result<Text<kMaxUtc>> et_to_utc(double et,
                                             UtcFormat format = UtcFormat::Isoc,
                                             int precision = 3) noexcept;

/// Target state relative to an observer. Needs an SPK (plus LSK/PCK as used).
[[nodiscard]] Result<State> state(std::string_view target, double et,
                                  std::string_view frame = "J2000",
                                  std::string_view abcorr = "NONE",
                                  std::string_view observer = "EARTH") noexcept;

/// Rotation matrix taking vectors in `from` into `to` at epoch `et`.
[[nodiscard]] Result<Matrix3> frame_transform(std::string_view from,
                                              std::string_view to,
                                              double et) noexcept;

}  // namespace cubesat::spice
