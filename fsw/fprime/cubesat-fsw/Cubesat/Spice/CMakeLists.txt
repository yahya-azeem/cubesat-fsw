# NAIF CSPICE is a prebuilt static toolkit, not an F´ module. The install
# script drops it in third_party/cspice (gitignored); point CSPICE_ROOT at a
# system install if you prefer.
set(CSPICE_ROOT "${CMAKE_SOURCE_DIR}/../../../third_party/cspice" CACHE PATH
    "NAIF CSPICE toolkit root (contains include/ and lib/)")

find_path(CSPICE_INCLUDE_DIR SpiceUsr.h
    HINTS "${CSPICE_ROOT}/include" NO_DEFAULT_PATH)
# NAIF ships lib/cspice.a (no "lib" prefix), so find_file, not find_library.
find_file(CSPICE_LIBRARY NAMES cspice.a libcspice.a
    HINTS "${CSPICE_ROOT}/lib" NO_DEFAULT_PATH)

if(NOT CSPICE_INCLUDE_DIR OR NOT CSPICE_LIBRARY)
    message(FATAL_ERROR
        "CSPICE not found under ${CSPICE_ROOT}.\n"
        "Run: ./setup/install-cspice.sh")
endif()

register_fprime_library(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/spice.cpp"
)

# The wrapper is C++23 and exception-free. PUBLIC propagates the standard to
# any component that includes spice.hpp.
target_compile_features(${FPRIME_CURRENT_MODULE} PUBLIC cxx_std_23)
target_compile_options(${FPRIME_CURRENT_MODULE} PRIVATE -fno-exceptions -fno-rtti)

# SYSTEM keeps CSPICE's C headers out of the project warning set.
target_include_directories(${FPRIME_CURRENT_MODULE} SYSTEM PUBLIC ${CSPICE_INCLUDE_DIR})
target_link_libraries(${FPRIME_CURRENT_MODULE} PUBLIC ${CSPICE_LIBRARY} m)
