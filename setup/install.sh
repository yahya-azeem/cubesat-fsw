#!/usr/bin/env bash
# Install the FOSS embedded toolchain on Arch / CachyOS.
# Run as your normal user (it calls sudo itself, and builds the AUR package
# as you rather than as root).
set -euo pipefail

if [[ $EUID -eq 0 ]]; then
    echo "Run this as your normal user, not root." >&2
    echo "It will call sudo where needed." >&2
    exit 1
fi

REPO_PKGS=(
    # AVR
    avr-gcc avr-libc avr-gdb avrdude
    # Emulation
    qemu-system-avr qemu-system-riscv qemu-system-xtensa
    # RISC-V cross toolchain
    riscv64-elf-gcc riscv64-elf-newlib riscv64-elf-binutils
    # Waveform viewing + simavr build/link deps
    gtkwave elfutils
    # Serial console (for real hardware)
    picocom
)

echo "==> installing repo packages"
sudo pacman -S --needed --noconfirm "${REPO_PKGS[@]}"

echo "==> installing simavr (AUR)"
if pacman -Q simavr >/dev/null 2>&1; then
    echo "simavr already installed"
elif command -v paru >/dev/null 2>&1; then
    paru -S --needed --noconfirm simavr
elif command -v yay >/dev/null 2>&1; then
    yay -S --needed --noconfirm simavr
else
    echo "No AUR helper found. Install simavr manually:" >&2
    echo "  git clone https://aur.archlinux.org/simavr.git /tmp/simavr && cd /tmp/simavr && makepkg -si" >&2
    exit 1
fi

echo "==> done. Now run: ./setup/verify.sh"
