#include "file_store.hpp"

#include <cstring>

namespace cubesat {

int ram_read(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
             void* buffer, lfs_size_t size)
{
    auto* dev = static_cast<RamBlock*>(cfg->context);
    if (dev == nullptr || block >= kBlockCount || off + size > kBlockSize) {
        return LFS_ERR_INVAL;
    }
    std::memcpy(buffer, dev->data.data() + (block * kBlockSize) + off, size);
    return LFS_ERR_OK;
}

int ram_program(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
                const void* buffer, lfs_size_t size)
{
    auto* dev = static_cast<RamBlock*>(cfg->context);
    if (dev == nullptr || block >= kBlockCount || off + size > kBlockSize) {
        return LFS_ERR_INVAL;
    }
    if (dev->fail_program_after == 0) {
        return LFS_ERR_IO;
    }
    if (dev->fail_program_after != UINT32_MAX) {
        --dev->fail_program_after;
    }
    std::memcpy(dev->data.data() + (block * kBlockSize) + off, buffer, size);
    return LFS_ERR_OK;
}

int ram_erase(const lfs_config* cfg, lfs_block_t block)
{
    auto* dev = static_cast<RamBlock*>(cfg->context);
    if (dev == nullptr || block >= kBlockCount) {
        return LFS_ERR_INVAL;
    }
    std::memset(dev->data.data() + (block * kBlockSize), 0xFF, kBlockSize);
    ++dev->erase_count;
    return LFS_ERR_OK;
}

int ram_sync(const lfs_config*) { return LFS_ERR_OK; }

namespace {

int nor_read(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
             void* buffer, lfs_size_t size)
{
    auto* dev = static_cast<SpiNorDevice*>(cfg->context);
    if (dev == nullptr || dev->ops.read == nullptr || block >= dev->sector_count ||
        off + size > dev->sector_size) {
        return LFS_ERR_INVAL;
    }
    const auto address =
        static_cast<std::uint32_t>(block) * dev->sector_size + static_cast<std::uint32_t>(off);
    return dev->ops.read(dev->ops.context, address, buffer, size) ? LFS_ERR_OK
                                                                  : LFS_ERR_IO;
}

int nor_program(const lfs_config* cfg, lfs_block_t block, lfs_off_t off,
                const void* buffer, lfs_size_t size)
{
    auto* dev = static_cast<SpiNorDevice*>(cfg->context);
    if (dev == nullptr || dev->ops.program == nullptr || block >= dev->sector_count ||
        off + size > dev->sector_size) {
        return LFS_ERR_INVAL;
    }
    const auto address =
        static_cast<std::uint32_t>(block) * dev->sector_size + static_cast<std::uint32_t>(off);
    return dev->ops.program(dev->ops.context, address, buffer, size) ? LFS_ERR_OK
                                                                     : LFS_ERR_IO;
}

int nor_erase(const lfs_config* cfg, lfs_block_t block)
{
    auto* dev = static_cast<SpiNorDevice*>(cfg->context);
    if (dev == nullptr || dev->ops.erase_sector == nullptr || block >= dev->sector_count) {
        return LFS_ERR_INVAL;
    }
    const auto address = static_cast<std::uint32_t>(block) * dev->sector_size;
    return dev->ops.erase_sector(dev->ops.context, address) ? LFS_ERR_OK : LFS_ERR_IO;
}

}  // namespace

FileStore::FileStore(SpiNorDevice& device) noexcept
{
    cfg_ = lfs_config{};
    cfg_.context = &device;
    cfg_.read = nor_read;
    cfg_.prog = nor_program;
    cfg_.erase = nor_erase;
    cfg_.sync = ram_sync;

    cfg_.read_size = 16;
    cfg_.prog_size = device.page_size;
    cfg_.block_size = device.sector_size;
    cfg_.block_count = device.sector_count;
    cfg_.block_cycles = 500;  // wear-level across ~500 erase cycles per block
    cfg_.cache_size = kCacheSize;
    cfg_.lookahead_size = kCacheSize;

    cfg_.read_buffer = read_cache_.data();
    cfg_.prog_buffer = prog_cache_.data();
    cfg_.lookahead_buffer = lookahead_.data();
}

FileStore::FileStore(RamBlock& device) noexcept
{
    cfg_ = lfs_config{};
    cfg_.context = &device;
    cfg_.read = ram_read;
    cfg_.prog = ram_program;
    cfg_.erase = ram_erase;
    cfg_.sync = ram_sync;

    cfg_.read_size = 16;
    cfg_.prog_size = 16;
    cfg_.block_size = kBlockSize;
    cfg_.block_count = kBlockCount;
    cfg_.block_cycles = 100;
    cfg_.cache_size = kCacheSize;
    cfg_.lookahead_size = kCacheSize;

    cfg_.read_buffer = read_cache_.data();
    cfg_.prog_buffer = prog_cache_.data();
    cfg_.lookahead_buffer = lookahead_.data();
}

FileStore::~FileStore() { unmount(); }

bool FileStore::mount() noexcept
{
    if (mounted_) {
        return true;
    }

    // Brand-new media has no superblock. Probe before mounting so a blank
    // device is formatted without littlefs logging a corruption error.
    std::array<std::uint8_t, 16> probe{};
    bool blank = cfg_.read != nullptr &&
                 cfg_.read(&cfg_, 0, 0, probe.data(), probe.size()) == LFS_ERR_OK;
    for (const auto byte : probe) {
        if (byte != 0xFF) {
            blank = false;
            break;
        }
    }

    if (blank) {
        if (lfs_format(&lfs_, &cfg_) != LFS_ERR_OK) {
            return false;
        }
    }

    // Fail closed on a non-blank device that will not mount: never silently
    // erase a filesystem that may still hold the only copy of the telemetry.
    const int err = lfs_mount(&lfs_, &cfg_);

    mounted_ = (err == LFS_ERR_OK);
    return mounted_;
}

void FileStore::unmount() noexcept
{
    if (mounted_) {
        static_cast<void>(lfs_unmount(&lfs_));
        mounted_ = false;
    }
}

lfs_ssize_t FileStore::write(const char* path, const void* data,
                             lfs_size_t size) noexcept
{
    if (!mounted_ || path == nullptr) {
        return LFS_ERR_INVAL;
    }

    lfs_file_t file{};
    int err = lfs_file_open(&lfs_, &file, path,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK) {
        return err;
    }

    const lfs_ssize_t written = lfs_file_write(&lfs_, &file, data, size);
    const int close_err = lfs_file_close(&lfs_, &file);

    if (written < 0) {
        return written;
    }
    if (close_err != LFS_ERR_OK) {
        return close_err;
    }
    // lfs_file_close committed the metadata transaction; a power cut before
    // this point leaves the previous version of the file intact.
    return written;
}

lfs_ssize_t FileStore::read(const char* path, void* buffer,
                            lfs_size_t capacity) noexcept
{
    if (!mounted_ || path == nullptr) {
        return LFS_ERR_INVAL;
    }

    lfs_file_t file{};
    int err = lfs_file_open(&lfs_, &file, path, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        return err;
    }

    const lfs_ssize_t got = lfs_file_read(&lfs_, &file, buffer, capacity);
    static_cast<void>(lfs_file_close(&lfs_, &file));
    return got;
}

lfs_ssize_t FileStore::append(const char* path, const void* data,
                              lfs_size_t size) noexcept
{
    if (!mounted_ || path == nullptr) {
        return LFS_ERR_INVAL;
    }

    lfs_file_t file{};
    int err = lfs_file_open(&lfs_, &file, path,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err != LFS_ERR_OK) {
        return err;
    }

    const lfs_ssize_t written = lfs_file_write(&lfs_, &file, data, size);
    const int close_err = lfs_file_close(&lfs_, &file);
    if (written < 0) {
        return written;
    }
    return close_err == LFS_ERR_OK ? written : close_err;
}

}  // namespace cubesat
