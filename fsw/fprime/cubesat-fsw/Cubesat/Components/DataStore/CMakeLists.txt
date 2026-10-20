register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/DataStore.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/DataStore.cpp"
    DEPENDS
        Cubesat_Storage
)

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/DataStore.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/DataStoreTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/DataStoreTester.cpp"
    DEPENDS
        Cubesat_Storage
    UT_AUTO_HELPERS
)
