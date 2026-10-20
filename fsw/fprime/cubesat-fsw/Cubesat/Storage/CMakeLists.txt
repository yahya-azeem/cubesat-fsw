# LittleFS is vendored (BSD-1-Clause) so the flight image has no external
# dependency and no network access at build time.
register_fprime_library(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/file_store.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/third_party/littlefs/lfs.c"
        "${CMAKE_CURRENT_LIST_DIR}/third_party/littlefs/lfs_util.c"
)
