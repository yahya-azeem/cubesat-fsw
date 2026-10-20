// Power/mode policy for the 1U CubeSat, written as a plain C++20 class so it
// compiles without the F´ autocoder and can be wrapped 1:1 by a passive
// F´ component (see fsw/README.md).
//
// No exceptions, no RTTI, no heap, no virtual calls.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace cubesat {

inline constexpr std::int32_t kDefaultReservePermille = 200;

/// mWh for `power_mw` held for `seconds`. Integer-only, truncating.
/// Prefer the milliwatt-second accounting inside [`Battery`]: 1.5 W for one
/// second is 0 mWh, which silently loses every 1 Hz charging tick.
[[nodiscard]] constexpr std::int32_t mwh(std::int32_t power_mw,
                                         std::int32_t seconds) noexcept
{
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(power_mw) * seconds) / 3600);
}

/// Milliwatt-seconds: the lossless internal energy unit.
[[nodiscard]] constexpr std::int64_t mws(std::int32_t power_mw,
                                         std::int32_t seconds) noexcept
{
    return static_cast<std::int64_t>(power_mw) * seconds;
}

enum class Mode : std::uint8_t { Safe, Charge, AwaitPass, Downlink };

enum class Event : std::uint8_t {
    FaultCleared,
    PassScheduled,
    PassCancelled,
    PassAcquired,
    DownlinkComplete,
    Fault,
    /// Liveness tick; never changes state.
    Tick,
};

class Supervisor {
public:
    constexpr Supervisor() noexcept = default;

    [[nodiscard]] constexpr Mode mode() const noexcept { return mode_; }
    [[nodiscard]] constexpr bool pa_permitted() const noexcept
    {
        return mode_ == Mode::Downlink;
    }

    /// Fail-closed transition. Unknown events hold state.
    Mode step(Event event) noexcept;

private:
    Mode mode_{Mode::Safe};
};

class Battery {
public:
    explicit constexpr Battery(std::int32_t capacity_mwh,
                               std::int32_t soc_permille = 0) noexcept
        : capacity_mws_{static_cast<std::int64_t>(capacity_mwh) * 3600},
          stored_mws_{capacity_mws_ * clamp_soc(soc_permille) / 1000}
    {
    }

    [[nodiscard]] constexpr std::int32_t capacity_mwh() const noexcept
    {
        return static_cast<std::int32_t>(capacity_mws_ / 3600);
    }
    [[nodiscard]] constexpr std::int32_t stored_mwh() const noexcept
    {
        return static_cast<std::int32_t>(stored_mws_ / 3600);
    }
    [[nodiscard]] constexpr std::int64_t stored_mws() const noexcept
    {
        return stored_mws_;
    }
    [[nodiscard]] constexpr std::int64_t reserve_mws() const noexcept
    {
        return capacity_mws_ * kDefaultReservePermille / 1000;
    }
    [[nodiscard]] constexpr std::int32_t reserve_mwh() const noexcept
    {
        return static_cast<std::int32_t>(reserve_mws() / 3600);
    }
    [[nodiscard]] constexpr std::int32_t soc_permille() const noexcept
    {
        return capacity_mws_ <= 0
                   ? 0
                   : static_cast<std::int32_t>((stored_mws_ * 1000) / capacity_mws_);
    }

    [[nodiscard]] constexpr bool can_afford(std::int32_t power_mw,
                                            std::int32_t seconds) const noexcept
    {
        return power_mw >= 0 && seconds >= 0 &&
               stored_mws_ - mws(power_mw, seconds) >= reserve_mws();
    }

    /// Returns the energy actually accepted, in milliwatt-seconds.
    std::int64_t charge(std::int32_t power_mw, std::int32_t seconds) noexcept;

    /// Spends only if the reserve survives.
    bool discharge(std::int32_t power_mw, std::int32_t seconds) noexcept;

private:
    static constexpr std::int32_t clamp_soc(std::int32_t v) noexcept
    {
        return v < 0 ? 0 : (v > 1000 ? 1000 : v);
    }

    std::int64_t capacity_mws_;
    std::int64_t stored_mws_;
};

// ---- CCSDS TM transfer-frame primary header -------------------------------

inline constexpr std::size_t kPrimaryHeaderLen = 6;
inline constexpr std::uint16_t kSeqUnpaged = 0b11;

struct PrimaryHeader {
    std::uint16_t apid;
    std::uint16_t sequence;
    std::uint16_t payload_len;

    friend constexpr bool operator==(const PrimaryHeader&,
                                     const PrimaryHeader&) = default;
};

/// Serialise into a caller-owned 6-byte buffer. `payload_len` must be >= 1.
void pack_primary_header(const PrimaryHeader& header,
                         std::span<std::uint8_t, kPrimaryHeaderLen> out) noexcept;

/// Parse, rejecting buffers shorter than the header.
[[nodiscard]] std::optional<PrimaryHeader>
parse_primary_header(std::span<const std::uint8_t> in) noexcept;

} // namespace cubesat
