SHELL := /bin/bash
.DEFAULT_GOAL := help

ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

MAKE_AVR   := $(MAKE) -C $(ROOT)/avr
MAKE_RISCV := $(MAKE) -C $(ROOT)/riscv
MAKE_FSW   := $(MAKE) -C $(ROOT)/fsw

FPRIME_DIR  := $(ROOT)/fsw/fprime/cubesat-fsw
FPRIME_VENV := $(ROOT)/.venv-fprime
FPRIME_SRC  := source $(FPRIME_VENV)/bin/activate &&
FPRIME_BIN  := $(FPRIME_DIR)/build-fprime-automatic-native/bin/Linux/CubesatDeployment

# Rust: firmware is Rust-first. C remains only for the AVR demo and vendor BSPs.
CARGO_RUST := cd $(ROOT)/rust && cargo
RV_TARGET  := riscv32imac-unknown-none-elf
ARM_TARGET := thumbv7em-none-eabihf

.PHONY: help all avr riscv rust fsw build clean sim vcd wave \
        debug debug-avr debug-riscv verify doctor esp32 \
        rust-test rust-riscv rust-arm rust-riscv-run rust-check rust-fmt \
        fprime fprime-run fprime-ut fprime-gds gds-demo cad cad-preview cad-check

help:
	@echo "embedded - headless FOSS toolchain (F´/C++ flight software)"
	@echo ""
	@echo "  make verify        check the installed toolchain"
	@echo "  make fsw           build + run the C++ flight-policy and LittleFS tests"
	@echo "  make fprime        build the F´ deployment (PowerManager + DataStore)"
	@echo "  make fprime-run    run the deployment headlessly for 5 s"
	@echo "  make fprime-ut     run the F´ component unit tests (11 tests)"
	@echo "  make fprime-gds    launch the browser GDS on http://localhost:5000"
	@echo "  make gds-demo      drive a full pass through the live GDS"
	@echo "  make cad           export 1U/exploded STL from cad/cubesat.scad"
	@echo "  make cad-preview   render the CAD PNGs"
	@echo "  make rust          test mission logic + build both Rust firmwares"
	@echo "  make rust-test     host unit tests for cubesat-core"
	@echo "  make rust-riscv    build rv32 firmware (QEMU virt)"
	@echo "  make rust-riscv-run run it headlessly in QEMU"
	@echo "  make rust-arm      build Cortex-M4 VA41620 skeleton"
	@echo ""
	@echo "  make avr           build AVR firmware (.elf + .hex, C demo)"
	@echo "  make riscv         build RISC-V C demo"
	@echo "  make build         build everything"
	@echo ""
	@echo "  make sim           run AVR firmware headlessly in simavr"
	@echo "  make vcd           run AVR firmware, dump pin timings to VCD"
	@echo "  make wave          open the VCD in gtkwave"
	@echo "  make debug-avr     simavr GDB stub + avr-gdb"
	@echo "  make run-riscv     run RISC-V C firmware in qemu (headless)"
	@echo "  make debug-riscv   qemu GDB stub + gdb"
	@echo ""
	@echo "  make clean         remove build artifacts"

all: build

build: rust fsw avr riscv

fsw:
	$(MAKE_FSW)

fprime:
	cd $(FPRIME_DIR) && $(FPRIME_SRC) fprime-util build -j$$(nproc)

fprime-run: fprime
	cd $(FPRIME_DIR) && timeout 5 $(FPRIME_BIN) < /dev/null || true

fprime-ut:
	@set -e; for c in PowerManager DataStore RadioDriver; do \
		echo "=== $$c ==="; \
		(cd $(FPRIME_DIR)/Cubesat/Components/$$c && $(FPRIME_SRC) fprime-util check); \
	done

fprime-gds: fprime
	cd $(FPRIME_DIR) && $(FPRIME_SRC) fprime-gds -d build-artifacts/Linux/CubesatDeployment --skip-browser-open

gds-demo:
	$(ROOT)/scripts/gds-demo.sh

# CAD: OpenSCAD 2021.01 or newer. Override with OPENSCAD=/path/to/AppImage.
OPENSCAD ?= openscad

cad:
	$(OPENSCAD) -o $(ROOT)/cad/cubesat.stl $(ROOT)/cad/cubesat.scad
	$(OPENSCAD) -D 'explode=0.55' -o $(ROOT)/cad/cubesat-exploded.stl $(ROOT)/cad/cubesat.scad

cad-preview:
	$(OPENSCAD) -o $(ROOT)/cad/cubesat.png --imgsize=1100,850 \
		--camera=0,0,0,55,0,25,0 --viewall --autocenter $(ROOT)/cad/cubesat.scad
	$(OPENSCAD) -D 'explode=0.55' -o $(ROOT)/cad/cubesat-exploded.png --imgsize=1100,850 \
		--camera=0,0,0,55,0,25,0 --viewall --autocenter $(ROOT)/cad/cubesat.scad
	$(OPENSCAD) -D 'show_panels=false' -D 'show_antenna=false' -D 'show_mli=false' \
		-D 'explode=0.9' -o $(ROOT)/cad/cubesat-stack.png --imgsize=1100,850 \
		--camera=0,0,0,78,0,18,0 --viewall --autocenter $(ROOT)/cad/cubesat.scad

cad-check: cad
	python3 $(ROOT)/cad/check_stl.py \
		$(ROOT)/cad/cubesat.stl $(ROOT)/cad/cubesat-exploded.stl

rust: rust-test rust-arm rust-riscv

rust-test:
	$(CARGO_RUST) test -p cubesat-core

rust-riscv:
	$(CARGO_RUST) build --release --target $(RV_TARGET) -p firmware-riscv-qemu

rust-arm:
	$(CARGO_RUST) build --release --target $(ARM_TARGET) -p firmware-va41620

rust-riscv-run: rust-riscv
	cd $(ROOT)/rust && timeout 5 cargo run --release --target $(RV_TARGET) -p firmware-riscv-qemu || true

rust-check:
	$(CARGO_RUST) clippy --workspace --all-targets -- -D warnings

rust-fmt:
	$(CARGO_RUST) fmt --all -- --check

avr:
	$(MAKE_AVR)

riscv:
	$(MAKE_RISCV)

sim:
	$(MAKE_AVR) sim

vcd:
	$(MAKE_AVR) vcd

wave:
	$(ROOT)/scripts/wave.sh

debug: debug-avr

debug-avr:
	$(MAKE_AVR) debug

debug-riscv:
	$(ROOT)/scripts/riscv-debug.sh

run-riscv:
	$(MAKE_RISCV) run

esp32:
	@cat $(ROOT)/esp32/README.md

verify doctor:
	$(ROOT)/setup/verify.sh

clean:
	$(MAKE_AVR) clean
	$(MAKE_RISCV) clean
	$(MAKE_FSW) clean
	rm -rf $(ROOT)/rust/target
