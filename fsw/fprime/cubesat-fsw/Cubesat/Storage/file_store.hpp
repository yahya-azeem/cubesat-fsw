// Power-fail-safe file storage on LittleFS.
//
// Two block backends: RAM (host tests / QEMU) and SPI NOR (flight). The
// LittleFS API and FileStore surface are identical for both.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// littlefs is vendored C: keep its warnings out of our -Werror build.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "third_party/littlefs/lfs.h"
#pragma GCC diagnostic pop

namespace cubesat {

inline constexpr lfs_size_t kBlockSize = 512;
inline constexpr lfs_size_t kBlockCount = 64;  // 32 KiB
/// Must be >= prog_size and a multiple of read_size.
inline constexpr lfs_size_t kCacheSize = 256;

/// In-memory block device used by the host tests and QEMU builds.
struct RamBlock {
    RamBlock() noexcept { data.fill(0xFF); }  // erased NOR state

    std::array<std::uint8_t, kBlockSize * kBlockCount> data{};
    std::uint32_t erase_count = 0;
    /// Program this many blocks successfully, then return LFS_ERR_IO.
    /// Used to simulate a brown-out mid-write.
    std::uint32_t fail_program_after = UINT32_MAX;
};

/// Transport for a real SPI NOR part (e.g. the VA41620 SPI port).
/// All addresses are byte offsets into the device.
struct SpiNorOps {
    void* context = nullptr;
    bool (*read)(void* context, std::uint32_t address, void* buffer,
                 std::size_t size) = nullptr;
    bool (*program)(void* context, std::uint32_t address, const void* buffer,
                    std::size_t size) = nullptr;
    bool (*erase_sector)(void* context, std::uint32_t sector_address) = nullptr;
};

/// Geometry of the attached NOR part. LittleFS blocks map 1:1 to sectors.
struct SpiNorDevice {
    SpiNorOps ops{};
    std::uint32_t sector_size = 4096;
    std::uint32_t sector_count = 1024;  // 4 MiB
    std::uint32_t page_size = 256;
};

int ram_read(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
             void* buffer, lfs_size_t size);
int ram_program(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
                const void* buffer, lfs_size_t size);
int ram_erase(const lfs_config* cfg, lfs_block_t block);
int ram_sync(const lfs_config* cfg);

class FileStore {
  public:
    explicit FileStore(RamBlock& device) noexcept;
    explicit FileStore(SpiNorDevice& device) noexcept;
    ~FileStore();

    FileStore(const FileStore&) = delete;
    FileStore& operator=(const FileStore&) = delete;

    /// Mounts, formatting only blank media. False if the device is unusable.
    bool mount() noexcept;
    void unmount() noexcept;

    /// Returns bytes written, or a negative lfs error.
    lfs_ssize_t write(const char* path, const void* data, lfs_size_t size) noexcept;

    /// Appends, creating the file if needed. Returns bytes written or < 0.
    lfs_ssize_t append(const char* path, const void* data, lfs_size_t size) noexcept;

    /// Returns bytes read, or a negative lfs error.
    lfs_ssize_t read(const char* path, void* buffer, lfs_size_t capacity) noexcept;

    [[nodiscard]] bool mounted() const noexcept { return mounted_; }

  private:
    lfs_t lfs_{};
    lfs_config cfg_{};
    bool mounted_ = false;

    alignas(8) std::array<std::uint8_t, kCacheSize> read_cache_{};
    alignas(8) std::array<std::uint8_t, kCacheSize> prog_cache_{};
    alignas(8) std::array<std::uint8_t, kCacheSize> lookahead_{};
};

}  // namespace cubesat
