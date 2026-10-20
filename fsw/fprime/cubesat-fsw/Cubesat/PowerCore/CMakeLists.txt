# Policy core: no F´ dependency on purpose, so the host tests can compile it
# directly and the component only wraps it.
register_fprime_library(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/power.cpp"
)
