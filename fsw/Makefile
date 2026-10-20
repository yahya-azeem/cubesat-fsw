CXX      ?= c++
CC       ?= cc
BUILD    := build
FW       := fprime/cubesat-fsw
CORE     := $(FW)/Cubesat/PowerCore
STORAGE  := $(FW)/Cubesat/Storage
LFS      := $(STORAGE)/third_party/littlefs

CXXFLAGS ?= -std=c++20 -O2 -g \
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
            -Werror -fno-exceptions -fno-rtti -fno-threadsafe-statics \
            -I$(FW)
STORAGE_CXXFLAGS := $(CXXFLAGS) -I$(STORAGE)
# Vendored C: build it strictly, but do not fail on third-party warnings.
# Logging is compiled out: flight has no stderr, and the corruption tests
# deliberately trigger error paths.
LFS_CFLAGS := -std=c17 -O2 -Wall -Wextra -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR

.PHONY: all test clean

all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/power.o: $(CORE)/power.cpp $(CORE)/power.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/file_store.o: $(STORAGE)/file_store.cpp $(STORAGE)/file_store.hpp | $(BUILD)
	$(CXX) $(STORAGE_CXXFLAGS) -c -o $@ $<

$(BUILD)/lfs.o: $(LFS)/lfs.c $(LFS)/lfs.h | $(BUILD)
	$(CC) $(LFS_CFLAGS) -c -o $@ $<

$(BUILD)/lfs_util.o: $(LFS)/lfs_util.c $(LFS)/lfs_util.h | $(BUILD)
	$(CC) $(LFS_CFLAGS) -c -o $@ $<

$(BUILD)/test_power: tests/test_power.cpp $(BUILD)/power.o | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $< $(BUILD)/power.o

$(BUILD)/test_storage: tests/test_storage.cpp $(BUILD)/file_store.o \
                       $(BUILD)/lfs.o $(BUILD)/lfs_util.o | $(BUILD)
	$(CXX) $(STORAGE_CXXFLAGS) -o $@ tests/test_storage.cpp \
		$(BUILD)/file_store.o $(BUILD)/lfs.o $(BUILD)/lfs_util.o

test: $(BUILD)/test_power $(BUILD)/test_storage
	./$(BUILD)/test_power
	./$(BUILD)/test_storage

clean:
	rm -rf $(BUILD)
