#!/usr/bin/env bash
# Attach avr-gdb to a live simavr instance.
# usage: avr-debug.sh firmware.elf [mcu] [freq_hz]
set -euo pipefail

elf=${1:?usage: avr-debug.sh firmware.elf [mcu] [freq_hz]}
mcu=${2:-atmega328p}
freq=${3:-16000000}

if ! command -v simavr >/dev/null; then
    echo "simavr not found - run setup/install.sh first" >&2
    exit 1
fi
if ! command -v avr-gdb >/dev/null; then
    echo "avr-gdb not found - run setup/install.sh first" >&2
    exit 1
fi

# simavr loads the ELF itself; -g opens the GDB stub on :1234.
simavr -g -m "$mcu" -f "$freq" "$elf" &
sim_pid=$!
trap 'kill "$sim_pid" 2>/dev/null || true' EXIT

# Give the stub a moment to bind.
for _ in $(seq 1 50); do
    if (echo >/dev/tcp/127.0.0.1/1234) 2>/dev/null; then break; fi
    sleep 0.1
done

exec avr-gdb -q "$elf" \
    -ex 'target remote localhost:1234' \
    -ex 'break main' \
    -ex 'continue'
