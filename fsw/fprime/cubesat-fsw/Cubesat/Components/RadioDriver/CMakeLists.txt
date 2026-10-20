register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RadioDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/RadioDriver.cpp"
)

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RadioDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RadioDriverTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RadioDriverTester.cpp"
    UT_AUTO_HELPERS
)
