// LittleFS integration tests: persistence, power loss, append, capacity,
// corruption handling and repeated mount cycles.
#include "file_store.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

int failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                        \
        }                                                                      \
    } while (false)

std::array<std::uint8_t, 200> pattern()
{
    std::array<std::uint8_t, 200> p{};
    for (std::size_t i = 0; i < p.size(); ++i) {
        p[i] = static_cast<std::uint8_t>((i * 7U) + 3U);
    }
    return p;
}

void test_unmounted_calls_fail()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(!fs.mounted());
    std::uint8_t byte = 0;
    CHECK(fs.write("x", &byte, 1) < 0);
    CHECK(fs.read("x", &byte, 1) < 0);
    CHECK(fs.append("x", &byte, 1) < 0);
}

void test_mount_is_idempotent()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());
    CHECK(fs.mounted());
    CHECK(fs.mount());  // second call is a no-op
    CHECK(fs.mounted());
}

void test_persistence()
{
    const auto expected = pattern();
    cubesat::RamBlock device;

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        CHECK(fs.write("telemetry.bin", expected.data(), expected.size()) ==
              static_cast<lfs_ssize_t>(expected.size()));
        fs.unmount();
        CHECK(!fs.mounted());
    }

    // Power cycle: fresh object, same flash contents.
    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        std::array<std::uint8_t, 200> actual{};
        CHECK(fs.read("telemetry.bin", actual.data(), actual.size()) ==
              static_cast<lfs_ssize_t>(actual.size()));
        CHECK(actual == expected);
    }
}

void test_power_loss_during_write()
{
    const auto expected = pattern();
    cubesat::RamBlock device;

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        CHECK(fs.write("telemetry.bin", expected.data(), expected.size()) ==
              static_cast<lfs_ssize_t>(expected.size()));
    }

    // Brown-out after one program: the new file must fail, the old one survive.
    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        device.fail_program_after = 1;
        std::array<std::uint8_t, 400> fresh{};
        fresh.fill(0xA5);
        CHECK(fs.write("downlink.tmp", fresh.data(), fresh.size()) < 0);
        device.fail_program_after = UINT32_MAX;
        fs.unmount();
    }

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        std::array<std::uint8_t, 200> actual{};
        CHECK(fs.read("telemetry.bin", actual.data(), actual.size()) ==
              static_cast<lfs_ssize_t>(actual.size()));
        CHECK(actual == expected);
    }
}

void test_append_and_remount()
{
    cubesat::RamBlock device;
    const std::array<std::uint8_t, 4> a{1, 2, 3, 4};
    const std::array<std::uint8_t, 3> b{5, 6, 7};

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        CHECK(fs.append("hk.log", a.data(), a.size()) == 4);
        CHECK(fs.append("hk.log", b.data(), b.size()) == 3);
    }

    // Remount, then keep appending: the file must continue, not truncate.
    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        CHECK(fs.append("hk.log", a.data(), a.size()) == 4);
    }

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        std::array<std::uint8_t, 16> out{};
        CHECK(fs.read("hk.log", out.data(), out.size()) == 11);
        const std::array<std::uint8_t, 11> expected{1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4};
        CHECK(std::memcmp(out.data(), expected.data(), expected.size()) == 0);
    }
}

void test_overwrite()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());

    const char v1[] = "housekeeping-1";
    const char v2[] = "housekeeping-2-longer";
    CHECK(fs.write("health.log", v1, sizeof(v1)) ==
          static_cast<lfs_ssize_t>(sizeof(v1)));
    CHECK(fs.write("health.log", v2, sizeof(v2)) ==
          static_cast<lfs_ssize_t>(sizeof(v2)));

    std::array<char, 64> out{};
    CHECK(fs.read("health.log", out.data(), out.size()) ==
          static_cast<lfs_ssize_t>(sizeof(v2)));
    CHECK(std::memcmp(out.data(), v2, sizeof(v2)) == 0);
}

void test_multiple_files_are_independent()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());

    const std::array<std::uint8_t, 3> a{9, 8, 7};
    const std::array<std::uint8_t, 5> b{1, 1, 2, 3, 5};
    CHECK(fs.write("a.bin", a.data(), a.size()) == 3);
    CHECK(fs.write("b.bin", b.data(), b.size()) == 5);

    std::array<std::uint8_t, 8> out{};
    CHECK(fs.read("a.bin", out.data(), out.size()) == 3);
    CHECK(std::memcmp(out.data(), a.data(), a.size()) == 0);
    CHECK(fs.read("b.bin", out.data(), out.size()) == 5);
    CHECK(std::memcmp(out.data(), b.data(), b.size()) == 0);
}

void test_large_file_spans_blocks()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());

    std::array<std::uint8_t, 4096> big{};
    for (std::size_t i = 0; i < big.size(); ++i) {
        big[i] = static_cast<std::uint8_t>(i % 251U);
    }
    CHECK(fs.write("payload.bin", big.data(), big.size()) ==
          static_cast<lfs_ssize_t>(big.size()));

    std::array<std::uint8_t, 4096> out{};
    CHECK(fs.read("payload.bin", out.data(), out.size()) ==
          static_cast<lfs_ssize_t>(out.size()));
    CHECK(out == big);

    // A short read must report only what was stored.
    std::array<std::uint8_t, 8> partial{};
    CHECK(fs.read("payload.bin", partial.data(), partial.size()) == 8);
    CHECK(std::memcmp(partial.data(), big.data(), 8) == 0);
}

void test_device_full_is_an_error_not_a_crash()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());

    std::array<std::uint8_t, 60000> too_big{};
    too_big.fill(0x5A);
    CHECK(fs.write("too-big.bin", too_big.data(), too_big.size()) < 0);

    // The filesystem must still be usable afterwards.
    const std::array<std::uint8_t, 4> small{1, 2, 3, 4};
    CHECK(fs.append("still-ok.bin", small.data(), small.size()) == 4);
}

void test_corruption_fails_closed()
{
    cubesat::RamBlock device;
    // Non-erased, non-filesystem media: mount must fail, and must not format.
    device.data.fill(0x00);

    cubesat::FileStore fs{device};
    CHECK(!fs.mount());
    CHECK(!fs.mounted());
    CHECK(device.data[0] == 0x00);  // no silent format
    CHECK(device.erase_count == 0);
}

void test_wear_leveling_touches_multiple_blocks()
{
    cubesat::RamBlock device;
    cubesat::FileStore fs{device};
    CHECK(fs.mount());

    std::array<std::uint8_t, 400> record{};
    record.fill(0x33);
    for (int i = 0; i < 40; ++i) {
        CHECK(fs.append("hk.log", record.data(), record.size()) ==
              static_cast<lfs_ssize_t>(record.size()));
    }

    // 40 * 400 bytes cannot fit in one 512-byte block, so littlefs must have
    // erased and rewritten several blocks.
    CHECK(device.erase_count > 1);

    std::array<std::uint8_t, 200> tail{};
    CHECK(fs.read("hk.log", tail.data(), tail.size()) ==
          static_cast<lfs_ssize_t>(tail.size()));
}

// ---- simulated SPI NOR ----------------------------------------------------

struct NorSim {
    static constexpr std::uint32_t kSectorSize = 4096;
    static constexpr std::uint32_t kSectors = 32;  // 128 KiB

    NorSim() noexcept { mem.fill(0xFF); }

    std::array<std::uint8_t, kSectorSize * kSectors> mem{};
    std::uint32_t erases = 0;
    std::uint32_t programs = 0;
    std::uint32_t violations = 0;  // 0->1 without erase, or page crossing
};

bool nor_read(void* context, std::uint32_t address, void* buffer, std::size_t size)
{
    auto* sim = static_cast<NorSim*>(context);
    if (address + size > sim->mem.size()) {
        return false;
    }
    std::memcpy(buffer, sim->mem.data() + address, size);
    return true;
}

bool nor_program(void* context, std::uint32_t address, const void* buffer,
                 std::size_t size)
{
    auto* sim = static_cast<NorSim*>(context);
    if (address + size > sim->mem.size()) {
        return false;
    }
    // Real NOR can only clear bits, and a program may not cross a 256-byte page.
    if ((address / 256U) != ((address + size - 1U) / 256U)) {
        ++sim->violations;
        return false;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(buffer);
    for (std::size_t i = 0; i < size; ++i) {
        if ((sim->mem[address + i] & bytes[i]) != bytes[i]) {
            ++sim->violations;  // tried to write a 1 over a 0
            return false;
        }
    }
    for (std::size_t i = 0; i < size; ++i) {
        sim->mem[address + i] &= bytes[i];
    }
    ++sim->programs;
    return true;
}

bool nor_erase(void* context, std::uint32_t sector_address)
{
    auto* sim = static_cast<NorSim*>(context);
    if (sector_address % NorSim::kSectorSize != 0 ||
        sector_address + NorSim::kSectorSize > sim->mem.size()) {
        return false;
    }
    std::memset(sim->mem.data() + sector_address, 0xFF, NorSim::kSectorSize);
    ++sim->erases;
    return true;
}

void test_spi_nor_backend()
{
    NorSim sim;
    cubesat::SpiNorDevice device;
    device.ops = cubesat::SpiNorOps{&sim, nor_read, nor_program, nor_erase};
    device.sector_size = NorSim::kSectorSize;
    device.sector_count = NorSim::kSectors;
    device.page_size = 256;

    std::array<std::uint8_t, 2048> payload{};
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>((i * 13U) + 1U);
    }

    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        CHECK(fs.write("nor.bin", payload.data(), payload.size()) ==
              static_cast<lfs_ssize_t>(payload.size()));
    }

    // Power cycle the part.
    {
        cubesat::FileStore fs{device};
        CHECK(fs.mount());
        std::array<std::uint8_t, 2048> out{};
        CHECK(fs.read("nor.bin", out.data(), out.size()) ==
              static_cast<lfs_ssize_t>(out.size()));
        CHECK(out == payload);

        // Sustained appends must churn sectors and never violate NOR rules.
        std::array<std::uint8_t, 8> record{1, 2, 3, 4, 5, 6, 7, 8};
        for (int i = 0; i < 128; ++i) {
            CHECK(fs.append("nor.log", record.data(), record.size()) == 8);
        }
    }

    CHECK(sim.erases > 1);
    CHECK(sim.programs > 0);
    CHECK(sim.violations == 0);
}

}  // namespace

int main()
{
    test_unmounted_calls_fail();
    test_mount_is_idempotent();
    test_persistence();
    test_power_loss_during_write();
    test_append_and_remount();
    test_overwrite();
    test_multiple_files_are_independent();
    test_large_file_spans_blocks();
    test_device_full_is_an_error_not_a_crash();
    test_corruption_fails_closed();
    test_wear_leveling_touches_multiple_blocks();
    test_spi_nor_backend();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("all LittleFS checks passed\n");
    return EXIT_SUCCESS;
}
