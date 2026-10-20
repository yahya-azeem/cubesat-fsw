// Host-side tests for the flight policy: energy accounting, the complete
// mode/event matrix, a multi-orbit mission simulation, and CCSDS framing.
// Dependency-free: a CHECK macro, no gtest, no exceptions.
#include "Cubesat/PowerCore/power.hpp"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <span>

namespace {

int failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                        \
        }                                                                      \
    } while (false)

using cubesat::Battery;
using cubesat::Event;
using cubesat::Mode;
using cubesat::PrimaryHeader;
using cubesat::Supervisor;
using cubesat::kPrimaryHeaderLen;

// ---------------------------------------------------------------- energy ----

void test_mwh_conversion()
{
    CHECK(cubesat::mwh(1000, 3600) == 1000);
    CHECK(cubesat::mwh(1500, 90 * 60) == 2250);
    CHECK(cubesat::mwh(13000, 300) == 1083);
    CHECK(cubesat::mwh(13000, 19) == 68);
    CHECK(cubesat::mwh(0, 3600) == 0);
    CHECK(cubesat::mwh(1, 1) == 0);        // truncates below 1 mWh
    CHECK(cubesat::mwh(100, 60) == 1);     // 1.66 mWh -> 1
    CHECK(cubesat::mwh(-100, 60) == -1);   // sign is preserved for callers
}

void test_battery_construction()
{
    const Battery empty{2250};
    CHECK(empty.capacity_mwh() == 2250);
    CHECK(empty.stored_mwh() == 0);
    CHECK(empty.soc_permille() == 0);

    const Battery full{2250, 1000};
    CHECK(full.stored_mwh() == 2250);
    CHECK(full.soc_permille() == 1000);

    CHECK((Battery{2250, 900}.stored_mwh() == 2025));
    CHECK((Battery{2250, -5}.stored_mwh() == 0));      // clamped low
    CHECK((Battery{2250, 5000}.stored_mwh() == 2250)); // clamped high
    CHECK((Battery{0, 500}.soc_permille() == 0));      // no divide by zero
}

void test_battery_reserve()
{
    Battery b{2000, 300};  // 600 stored, 400 reserve
    CHECK(b.reserve_mwh() == 400);
    CHECK(b.can_afford(0, 0));
    CHECK(b.can_afford(100, 60));    // 1 mWh, fine
    CHECK(!b.can_afford(3000, 600)); // 500 mWh would cross the reserve

    CHECK(b.discharge(3000, 600) == false);
    CHECK(b.stored_mwh() == 600);    // failed discharge is atomic

    CHECK(b.discharge(100, 60));
    CHECK(b.stored_mwh() == 598);
}

void test_battery_charge()
{
    Battery b{1000};
    CHECK(b.charge(0, 100) == 0);
    CHECK(b.charge(100, 0) == 0);
    CHECK(b.charge(-10, 100) == 0);
    CHECK(b.charge(10, -100) == 0);
    CHECK(b.stored_mwh() == 0);

    CHECK(b.charge(500, 3600) == 1'800'000);
    CHECK(b.stored_mwh() == 500);
    CHECK(b.charge(10'000, 3600) == 1'800'000);  // saturates at capacity
    CHECK(b.stored_mwh() == 1000);
    CHECK(b.soc_permille() == 1000);
    CHECK(b.charge(10'000, 3600) == 0);
}

void test_battery_negative_discharge()
{
    Battery b{1000, 1000};
    CHECK(!b.discharge(-1, 10));
    CHECK(!b.can_afford(-1, 10));
    CHECK(b.stored_mwh() == 1000);
}

// ---------------------------------------------------------------- modes ----

Mode reach(Mode target)
{
    Supervisor s;
    if (target == Mode::Safe) {
        return s.mode();
    }
    static_cast<void>(s.step(Event::FaultCleared));
    if (target == Mode::Charge) {
        return s.mode();
    }
    static_cast<void>(s.step(Event::PassScheduled));
    if (target == Mode::AwaitPass) {
        return s.mode();
    }
    static_cast<void>(s.step(Event::PassAcquired));
    return s.mode();
}

void test_mode_reachability()
{
    CHECK(reach(Mode::Safe) == Mode::Safe);
    CHECK(reach(Mode::Charge) == Mode::Charge);
    CHECK(reach(Mode::AwaitPass) == Mode::AwaitPass);
    CHECK(reach(Mode::Downlink) == Mode::Downlink);
}

void test_full_transition_matrix()
{
    struct Case {
        Mode from;
        Event event;
        Mode expected;
    };

    constexpr std::array<Mode, 4> modes{Mode::Safe, Mode::Charge, Mode::AwaitPass,
                                        Mode::Downlink};
    constexpr std::array<Event, 7> events{
        Event::FaultCleared, Event::PassScheduled, Event::PassCancelled,
        Event::PassAcquired, Event::DownlinkComplete, Event::Fault, Event::Tick};

    for (const Mode from : modes) {
        for (const Event event : events) {
            Supervisor s;
            CHECK(s.step(Event::FaultCleared) == Mode::Charge);
            // Drive to `from` without using the event under test.
            if (from == Mode::Safe) {
                s = Supervisor{};
            } else if (from == Mode::AwaitPass) {
                CHECK(s.step(Event::PassScheduled) == Mode::AwaitPass);
            } else if (from == Mode::Downlink) {
                CHECK(s.step(Event::PassScheduled) == Mode::AwaitPass);
                CHECK(s.step(Event::PassAcquired) == Mode::Downlink);
            }
            CHECK(s.mode() == from);

            Mode expected = from;
            if (event == Event::Fault) {
                expected = Mode::Safe;
            } else if (from == Mode::Safe && event == Event::FaultCleared) {
                expected = Mode::Charge;
            } else if (from == Mode::Charge && event == Event::PassScheduled) {
                expected = Mode::AwaitPass;
            } else if (from == Mode::AwaitPass && event == Event::PassAcquired) {
                expected = Mode::Downlink;
            } else if (from == Mode::AwaitPass && event == Event::PassCancelled) {
                expected = Mode::Charge;
            } else if (from == Mode::Downlink && event == Event::DownlinkComplete) {
                expected = Mode::Charge;
            } else if (from == Mode::Downlink && event == Event::PassCancelled) {
                expected = Mode::AwaitPass;
            }

            CHECK(s.step(event) == expected);
        }
    }
}

void test_pa_gate()
{
    Supervisor s;
    CHECK(!s.pa_permitted());

    static_cast<void>(s.step(Event::FaultCleared));
    CHECK(!s.pa_permitted());
    static_cast<void>(s.step(Event::PassScheduled));
    CHECK(!s.pa_permitted());
    static_cast<void>(s.step(Event::PassAcquired));
    CHECK(s.pa_permitted());
    static_cast<void>(s.step(Event::PassCancelled));
    CHECK(!s.pa_permitted());
}

void test_redundant_events_hold_state()
{
    Supervisor s;
    for (int i = 0; i < 10; ++i) {
        CHECK(s.step(Event::Tick) == Mode::Safe);
        CHECK(s.step(Event::PassAcquired) == Mode::Safe);
    }
    static_cast<void>(s.step(Event::FaultCleared));
    for (int i = 0; i < 10; ++i) {
        CHECK(s.step(Event::FaultCleared) == Mode::Charge);
        CHECK(s.step(Event::DownlinkComplete) == Mode::Charge);
    }
}

// ------------------------------------------------------------- mission ----

void test_four_orbit_steady_state()
{
    constexpr std::int32_t kOrbitSeconds = 90 * 60;
    constexpr std::int32_t kPassSeconds = 300;

    Battery battery{2250, 900};
    Supervisor supervisor;
    CHECK(supervisor.step(Event::FaultCleared) == Mode::Charge);

    for (int orbit = 0; orbit < 4; ++orbit) {
        // 1 Hz charging ticks during the sunlit coast.
        for (std::int32_t t = 0; t < kOrbitSeconds; ++t) {
            static_cast<void>(battery.charge(1500, 1));
        }
        CHECK(battery.stored_mwh() == 2250);  // array keeps up with the load

        CHECK(supervisor.step(Event::PassScheduled) == Mode::AwaitPass);
        CHECK(supervisor.step(Event::PassAcquired) == Mode::Downlink);
        CHECK(supervisor.pa_permitted());
        CHECK(battery.can_afford(13'000, kPassSeconds));
        CHECK(battery.discharge(13'000, kPassSeconds));
        CHECK(supervisor.step(Event::DownlinkComplete) == Mode::Charge);
        CHECK(!supervisor.pa_permitted());

        CHECK(battery.stored_mwh() >= battery.reserve_mwh());
        CHECK(battery.stored_mwh() == 1166);  // 2250 - 1083.33, truncated
    }
}

void test_guard_blocks_burst_at_low_soc()
{
    Battery battery{2250, 220};  // 495 mWh stored, 450 reserve
    CHECK(!battery.can_afford(13'000, 300));
    CHECK(!battery.discharge(13'000, 300));
    CHECK(battery.stored_mwh() == 495);
}

// ---------------------------------------------------------------- frames ----

void test_ccsds_roundtrip_sweep()
{
    constexpr std::array<std::uint16_t, 8> apids{0, 1, 0x7FF, 0x123, 0x456,
                                                 0x7FE, 0x555, 0x2AA};
    constexpr std::array<std::uint16_t, 6> seqs{0, 1, 0x3FFF, 0x2A5, 0x1000, 0x0F0F};

    for (const auto apid : apids) {
        for (const auto seq : seqs) {
            const PrimaryHeader in{apid, seq, 1};
            std::array<std::uint8_t, kPrimaryHeaderLen> buf{};
            cubesat::pack_primary_header(in, buf);
            const auto out = cubesat::parse_primary_header(buf);
            CHECK(out.has_value());
            CHECK(out == in);
        }
    }
}

void test_ccsds_length_field()
{
    std::array<std::uint8_t, kPrimaryHeaderLen> buf{};

    cubesat::pack_primary_header(PrimaryHeader{1, 0, 1}, buf);
    CHECK(buf[4] == 0 && buf[5] == 0);  // length field = payload - 1

    cubesat::pack_primary_header(PrimaryHeader{1, 0, 256}, buf);
    CHECK(buf[4] == 0 && buf[5] == 0xFF);

    cubesat::pack_primary_header(PrimaryHeader{1, 0, 65535}, buf);
    CHECK(buf[4] == 0xFF && buf[5] == 0xFE);

    constexpr std::array<std::uint16_t, 7> lengths{1, 2, 255, 256, 1024, 4096, 65535};
    for (const std::uint16_t len : lengths) {
        cubesat::pack_primary_header(PrimaryHeader{7, 9, len}, buf);
        const auto parsed = cubesat::parse_primary_header(buf);
        CHECK(parsed.has_value());
        CHECK(parsed->payload_len == len);
    }
}

void test_ccsds_masking_and_short_buffers()
{
    std::array<std::uint8_t, kPrimaryHeaderLen> buf{};
    cubesat::pack_primary_header(PrimaryHeader{0xFFFF, 0xFFFF, 1}, buf);

    // bits 0-2 version, 3 type, 4 sec-hdr must be zero; 11 APID bits.
    CHECK((buf[0] & 0xF8U) == 0);
    CHECK(buf[0] == 0x07);
    CHECK(buf[2] == 0xFF);  // seq flags 11 + high 6 count bits
    const auto parsed = cubesat::parse_primary_header(buf);
    CHECK(parsed.has_value());
    CHECK(parsed->apid == 0x7FF);
    CHECK(parsed->sequence == 0x3FFF);

    for (std::size_t n = 0; n < kPrimaryHeaderLen; ++n) {
        const std::array<std::uint8_t, kPrimaryHeaderLen> shortbuf{};
        CHECK(!cubesat::parse_primary_header(
                   std::span<const std::uint8_t>(shortbuf.data(), n))
                   .has_value());
    }
}

}  // namespace

int main()
{
    test_mwh_conversion();
    test_battery_construction();
    test_battery_reserve();
    test_battery_charge();
    test_battery_negative_discharge();
    test_mode_reachability();
    test_full_transition_matrix();
    test_pa_gate();
    test_redundant_events_hold_state();
    test_four_orbit_steady_state();
    test_guard_blocks_burst_at_low_soc();
    test_ccsds_roundtrip_sweep();
    test_ccsds_length_field();
    test_ccsds_masking_and_short_buffers();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("all C++ policy checks passed\n");
    return EXIT_SUCCESS;
}
