#!/usr/bin/env bash
# Live RISC-V debugging: start QEMU halted with a GDB stub on :1234.
# usage: riscv-debug.sh [firmware.elf]
set -euo pipefail

cd "$(dirname "$0")/.."
elf=${1:-riscv/build/firmware.elf}

if [[ ! -f $elf ]]; then
    make -s riscv
fi

echo "starting qemu-system-riscv32 (halted, gdb :1234)"
qemu-system-riscv32 -machine virt -bios none -kernel "$elf" \
    -nographic -monitor none -S -gdb tcp::1234 &
qemu_pid=$!
trap 'kill "$qemu_pid" 2>/dev/null || true' EXIT

sleep 0.3
gdb -q "$elf" \
    -ex 'set architecture riscv:rv32' \
    -ex 'target remote localhost:1234' \
    -ex 'break main' \
    -ex 'continue'
