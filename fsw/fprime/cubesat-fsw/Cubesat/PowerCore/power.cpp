#include "power.hpp"

namespace cubesat {

Mode Supervisor::step(Event event) noexcept
{
    using enum Mode;

    // Faults always win and are sticky until explicitly cleared.
    if (event == Event::Fault) {
        mode_ = Safe;
        return mode_;
    }

    switch (mode_) {
    case Safe:
        if (event == Event::FaultCleared) {
            mode_ = Charge;
        }
        break;
    case Charge:
        if (event == Event::PassScheduled) {
            mode_ = AwaitPass;
        }
        break;
    case AwaitPass:
        if (event == Event::PassAcquired) {
            mode_ = Downlink;
        } else if (event == Event::PassCancelled) {
            mode_ = Charge;
        }
        break;
    case Downlink:
        if (event == Event::DownlinkComplete) {
            mode_ = Charge;
        } else if (event == Event::PassCancelled) {
            // Lost the pass before the queue drained.
            mode_ = AwaitPass;
        }
        break;
    }
    return mode_;
}

std::int64_t Battery::charge(std::int32_t power_mw,
                             std::int32_t seconds) noexcept
{
    if (power_mw <= 0 || seconds <= 0) {
        return 0;
    }
    const std::int64_t requested = mws(power_mw, seconds);
    const std::int64_t free = capacity_mws_ - stored_mws_;
    const std::int64_t accepted = requested < free ? requested : free;
    stored_mws_ += accepted;
    return accepted;
}

bool Battery::discharge(std::int32_t power_mw, std::int32_t seconds) noexcept
{
    if (power_mw < 0 || seconds < 0) {
        return false;
    }
    const std::int64_t need = mws(power_mw, seconds);
    if (stored_mws_ - need < reserve_mws()) {
        return false;
    }
    stored_mws_ -= need;
    return true;
}

void pack_primary_header(const PrimaryHeader& header,
                         std::span<std::uint8_t, kPrimaryHeaderLen> out) noexcept
{
    const auto apid = static_cast<std::uint16_t>(header.apid & 0x07FFU);
    const auto seq = static_cast<std::uint16_t>(header.sequence & 0x3FFFU);
    const auto len = static_cast<std::uint16_t>(
        header.payload_len == 0 ? 0 : header.payload_len - 1);

    out[0] = static_cast<std::uint8_t>(apid >> 8);
    out[1] = static_cast<std::uint8_t>(apid & 0xFFU);
    out[2] = static_cast<std::uint8_t>((kSeqUnpaged << 6) | (seq >> 8));
    out[3] = static_cast<std::uint8_t>(seq & 0xFFU);
    out[4] = static_cast<std::uint8_t>(len >> 8);
    out[5] = static_cast<std::uint8_t>(len & 0xFFU);
}

std::optional<PrimaryHeader>
parse_primary_header(std::span<const std::uint8_t> in) noexcept
{
    if (in.size() < kPrimaryHeaderLen) {
        return std::nullopt;
    }
    const auto apid = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(in[0] & 0x07U) << 8) | in[1]);
    const auto seq = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(in[2] & 0x3FU) << 8) | in[3]);
    const auto len =
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(in[4]) << 8) | in[5]);

    return PrimaryHeader{apid, seq, static_cast<std::uint16_t>(len + 1U)};
}

} // namespace cubesat
