// Host tests for the CSPICE wrapper: bounded strings, error recovery,
// time conversions, synthetic ephemeris kernels and frame invariants.
#include "Cubesat/Spice/spice.hpp"

#include <SpiceUsr.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace {

int failures = 0;
std::string kernel_dir;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                        \
        }                                                                      \
    } while (false)

#define CHECK_OK(expr)                                                         \
    do {                                                                       \
        auto result = (expr);                                                  \
        if (!result) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d  %s -> %.*s\n", __FILE__, __LINE__, \
                         #expr, static_cast<int>(result.error().message.length), \
                         result.error().message.c_str());                      \
            ++failures;                                                        \
        }                                                                      \
    } while (false)

#define CHECK_ERR(expr, expected)                                              \
    do {                                                                       \
        auto result = (expr);                                                  \
        if (result) {                                                          \
            std::fprintf(stderr, "FAIL %s:%d  expected error\n", __FILE__, __LINE__); \
            ++failures;                                                        \
        } else if (result.error().code != (expected)) {                        \
            std::fprintf(stderr, "FAIL %s:%d  wrong error code\n", __FILE__, __LINE__); \
            ++failures;                                                        \
        }                                                                      \
    } while (false)

bool write_test_spk(const char* path)
{
    std::remove(path);
    cubesat::spice::initialize();

    SpiceInt handle = 0;
    SpiceChar ifname[] = "CUBESAT_SPICE_TEST";
    spkopn_c(path, ifname, 0, &handle);
    if (failed_c() != 0) {
        reset_c();
        return false;
    }

    constexpr int kRecords = 11;
    std::array<SpiceDouble, kRecords * 6> states{};
    for (int i = 0; i < kRecords; ++i) {
        const double t = 10.0 * i;
        const auto base = static_cast<std::size_t>(i) * 6U;
        states[base + 0U] = 1000.0 + t;
        states[base + 1U] = 2000.0;
        states[base + 2U] = 3000.0;
        states[base + 3U] = 10.0;
        states[base + 4U] = 0.0;
        states[base + 5U] = 0.0;
    }

    SpiceChar frame[] = "J2000";
    SpiceChar segid[] = "TEST_SEG";
    spkw08_c(handle, -999, 399, frame, 0.0, 100.0, segid, 1, kRecords,
             states.data(), 0.0, 10.0);
    const bool wrote = failed_c() == 0;
    reset_c();

    spkcls_c(handle);
    const bool closed = failed_c() == 0;
    reset_c();

    return wrote && closed;
}

void test_time()
{
    using namespace cubesat::spice;

    Kernel lsk;
    CHECK_OK(lsk.load(kernel_dir + "/naif0012.tls"));

    auto j2000 = utc_to_et("2000-01-01T11:58:55.816");
    CHECK_OK(j2000);
    if (j2000) {
        CHECK(std::abs(*j2000) < 1.0e-3);
    }

    auto et = utc_to_et("2026-11-12T00:00:00");
    CHECK_OK(et);
    if (et) {
        CHECK(*et > 8.4e8);
    }

    auto isoc = et_to_utc(0.0, UtcFormat::Isoc, 3);
    CHECK_OK(isoc);
    if (isoc) {
        CHECK(isoc->view().starts_with("2000-01-01T11:58:55"));
        CHECK(isoc->length < kMaxUtc);
    }

    if (et) {
        auto back = et_to_utc(*et, UtcFormat::Isoc, 3);
        CHECK_OK(back);
        if (back) {
            auto round = utc_to_et(back->view());
            CHECK_OK(round);
            if (round) {
                CHECK(std::abs(*round - *et) < 2.0e-3);
            }
        }
    }

    auto cals = et_to_utc(0.0, UtcFormat::Cals, 0);
    CHECK_OK(cals);
    if (cals) {
        CHECK(cals->view().find("2000") != std::string_view::npos);
    }
}

void test_time_errors()
{
    using namespace cubesat::spice;

    Kernel lsk;
    CHECK_OK(lsk.load(kernel_dir + "/naif0012.tls"));

    CHECK_ERR(utc_to_et("not-a-date"), Code::TimeConversion);
    CHECK_ERR(utc_to_et(""), Code::InvalidArgument);
    CHECK_ERR(utc_to_et(std::string(200, '2')), Code::InvalidArgument);
    CHECK_ERR(et_to_utc(0.0, UtcFormat::Isoc, -1), Code::InvalidArgument);
    CHECK_ERR(et_to_utc(0.0, UtcFormat::Isoc, 10), Code::InvalidArgument);
    CHECK_ERR(et_to_utc(std::numeric_limits<double>::quiet_NaN()),
              Code::NonFinite);
    CHECK_ERR(et_to_utc(std::numeric_limits<double>::infinity()),
              Code::NonFinite);

    std::string with_nul("2026-01-01");
    with_nul.push_back('\0');
    CHECK_ERR(utc_to_et(with_nul), Code::InvalidArgument);

    // A failed call must reset CSPICE, so the next one succeeds.
    auto good = utc_to_et("2026-01-01T00:00:00");
    CHECK_OK(good);
}

void test_kernel_errors()
{
    using namespace cubesat::spice;

    Kernel missing;
    CHECK_ERR(missing.load("/no/such/kernel.tls"), Code::KernelLoad);
    CHECK(!missing.loaded());

    Kernel too_long;
    CHECK_ERR(too_long.load(std::string(kMaxPath, 'k')), Code::InvalidArgument);
    CHECK(!too_long.load(std::string{}));

    Kernel lsk;
    CHECK_OK(lsk.load(kernel_dir + "/naif0012.tls"));
    CHECK(lsk.loaded());

    Kernel moved(std::move(lsk));
    CHECK(moved.loaded());
    CHECK(!lsk.loaded());
    CHECK_OK(moved.unload());
    CHECK(!moved.loaded());

    // With the leapseconds kernel gone, conversions fail closed.
    CHECK_ERR(utc_to_et("2026-01-01T00:00:00"), Code::TimeConversion);

    CHECK_OK(moved.load(kernel_dir + "/naif0012.tls"));
    CHECK_OK(utc_to_et("2026-01-01T00:00:00"));
}

void test_ephemeris()
{
    using namespace cubesat::spice;

    constexpr const char* kPath = "/tmp/cubesat_spice_test.spk";
    CHECK(write_test_spk(kPath));

    Kernel spk;
    CHECK_OK(spk.load(kPath));

    auto at_mid = state("-999", 55.0, "J2000", "NONE", "399");
    CHECK_OK(at_mid);
    if (at_mid) {
        CHECK(std::abs(at_mid->position_km[0] - 1055.0) < 1.0e-6);
        CHECK(std::abs(at_mid->position_km[1] - 2000.0) < 1.0e-6);
        CHECK(std::abs(at_mid->position_km[2] - 3000.0) < 1.0e-6);
        CHECK(std::abs(at_mid->velocity_km_s[0] - 10.0) < 1.0e-6);
        CHECK(std::abs(at_mid->velocity_km_s[1]) < 1.0e-9);
        CHECK(std::isfinite(at_mid->light_time_s));
    }

    CHECK_ERR(state("-999", 200.0, "J2000", "NONE", "399"), Code::Ephemeris);
    CHECK_ERR(state("-999", std::numeric_limits<double>::quiet_NaN()), Code::NonFinite);
    CHECK_ERR(state("NO_SUCH_BODY", 0.0), Code::Ephemeris);

    // Time Coverage failure must not poison later calls.
    auto inside = state("-999", 0.0, "J2000", "NONE", "399");
    CHECK_OK(inside);
    if (inside) {
        CHECK(std::abs(inside->position_km[0] - 1000.0) < 1.0e-6);
    }

    std::remove(kPath);
}

void test_frames()
{
    using namespace cubesat::spice;

    Kernel pck;
    CHECK_OK(pck.load(kernel_dir + "/pck00011.tpc"));

    auto identity = frame_transform("J2000", "J2000", 0.0);
    CHECK_OK(identity);
    if (identity) {
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                CHECK(std::abs(identity->m[r][c] - (r == c ? 1.0 : 0.0)) < 1.0e-15);
            }
        }
    }

    auto earth = frame_transform("J2000", "IAU_EARTH", 0.0);
    CHECK_OK(earth);
    if (earth) {
        double determinant = 0.0;
        const auto& m = earth->m;
        determinant =
            (m[0][0] * ((m[1][1] * m[2][2]) - (m[1][2] * m[2][1]))) -
            (m[0][1] * ((m[1][0] * m[2][2]) - (m[1][2] * m[2][0]))) +
            (m[0][2] * ((m[1][0] * m[2][1]) - (m[1][1] * m[2][0])));
        CHECK(std::abs(determinant - 1.0) < 1.0e-12);

        auto inverse = frame_transform("IAU_EARTH", "J2000", 0.0);
        CHECK_OK(inverse);
        if (inverse) {
            for (std::size_t r = 0; r < 3; ++r) {
                for (std::size_t c = 0; c < 3; ++c) {
                    CHECK(std::abs(inverse->m[r][c] - m[c][r]) < 1.0e-12);
                }
            }
        }
    }

    CHECK_ERR(frame_transform("", "J2000", 0.0), Code::InvalidArgument);
    CHECK_ERR(frame_transform("J2000", "J2000", std::numeric_limits<double>::infinity()),
              Code::NonFinite);
}

}  // namespace

int main(int argc, char** argv)
{
    kernel_dir = (argc > 1) ? argv[1]
                            : "fprime/cubesat-fsw/Cubesat/Spice/kernels";

    test_time();
    test_time_errors();
    test_kernel_errors();
    test_ephemeris();
    test_frames();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("all CSPICE checks passed\n");
    return EXIT_SUCCESS;
}
