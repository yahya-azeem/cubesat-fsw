# Absolute kernel directory shared by the flight target and the UT.
set(ASTRONAV_KERNEL_DIR "${CMAKE_SOURCE_DIR}/Cubesat/Spice/kernels" CACHE PATH
    "Directory containing the CSPICE kernels used by AstroNav")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/kernel_dir.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/kernel_dir.hpp"
    @ONLY
)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/AstroNav.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/AstroNav.cpp"
    DEPENDS
        Cubesat_Spice
)

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/AstroNav.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/AstroNavTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/AstroNavTester.cpp"
    DEPENDS
        Cubesat_Spice
    UT_AUTO_HELPERS
)
