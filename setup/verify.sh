#!/usr/bin/env bash
# Report which pieces of the toolchain are present and working.
set -uo pipefail

ok=0
bad=0

have() { command -v "$1" >/dev/null 2>&1; }

check() {
    local name=$1 cmd=$2
    if have "$cmd"; then
        printf '  \033[32mok\033[0m    %-24s %s\n' "$name" "$(command -v "$cmd")"
        ok=$((ok + 1))
    else
        printf '  \033[31mMISS\033[0m  %-24s %s\n' "$name" "$cmd not found"
        bad=$((bad + 1))
    fi
}

echo "== compilers & debuggers"
check "clang"          clang
check "avr-gcc"        avr-gcc
check "avr-gdb"        avr-gdb
check "avrdude"        avrdude
check "gdb"            gdb
check "make"           make
check "cmake"          cmake

echo "== RISC-V cross toolchain"
if have riscv64-unknown-elf-gcc; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "riscv64-gcc" "$(command -v riscv64-unknown-elf-gcc)"
    ok=$((ok + 1))
elif have riscv64-elf-gcc; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "riscv64-gcc" "$(command -v riscv64-elf-gcc)"
    ok=$((ok + 1))
else
    printf '  \033[31mMISS\033[0m  %-24s riscv64 gcc not found\n' "riscv64-gcc"
    bad=$((bad + 1))
fi

echo "== rust"
check "rustc" rustc
check "cargo" cargo
if have rustup; then
    for t in thumbv7em-none-eabihf riscv32imac-unknown-none-elf riscv32imc-unknown-none-elf; do
        if rustup target list --installed 2>/dev/null | grep -qx "$t"; then
            printf '  \033[32mok\033[0m    %-24s %s\n' "target" "$t"
            ok=$((ok + 1))
        else
            printf '  \033[31mMISS\033[0m  %-24s %s\n' "target" "$t (rustup target add $t)"
            bad=$((bad + 1))
        fi
    done
fi

echo "== emulators"
check "simavr"             simavr
check "qemu-system-avr"    qemu-system-avr
check "qemu-system-riscv32" qemu-system-riscv32
check "qemu-system-xtensa" qemu-system-xtensa
check "gtkwave"            gtkwave

echo "== libraries & features"
if pkg-config --exists simavr 2>/dev/null || [[ -e /usr/lib/libsimavr.so ]]; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "libsimavr" "library + pkg-config present"
    ok=$((ok + 1))
else
    printf '  \033[31mMISS\033[0m  %-24s %s\n' "libsimavr" "headers/libs missing"
    bad=$((bad + 1))
fi

if [[ -f /usr/include/simavr/sim_avr.h ]]; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "simavr headers" "/usr/include/simavr"
    ok=$((ok + 1))
else
    printf '  \033[31mMISS\033[0m  %-24s %s\n' "simavr headers" "/usr/include/simavr/sim_avr.h missing"
    bad=$((bad + 1))
fi

if have qemu-system-xtensa && qemu-system-xtensa -M help 2>/dev/null | grep -qw esp32; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "qemu esp32 machine" "available"
else
    printf '  \033[33mnote\033[0m  %-24s %s\n' "qemu esp32 machine" "not in this QEMU build"
fi

if have qemu-system-riscv32 && qemu-system-riscv32 -M help 2>/dev/null | grep -qw esp32c3; then
    printf '  \033[32mok\033[0m    %-24s %s\n' "qemu esp32c3 machine" "available"
else
    printf '  \033[33mnote\033[0m  %-24s %s\n' "qemu esp32c3 machine" "not in this QEMU build"
fi

echo
echo "== gdb targets"
if have gdb; then
    if gdb --configuration 2>/dev/null | grep -q 'enable-targets=all'; then
        echo "  gdb supports all architectures (riscv + avr via avr-gdb)"
    else
        echo "  gdb is a single-target build; riscv debugging may need gdb-multiarch"
    fi
fi

echo
if (( bad == 0 )); then
    printf '\033[32m%s checks passed.\033[0m\n' "$ok"
else
    printf '\033[31m%d missing\033[0m, %d ok. Run ./setup/install.sh\n' "$bad" "$ok"
    exit 1
fi
