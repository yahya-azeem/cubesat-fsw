# ESP32 / ESP32-C3 emulation

**Mainline QEMU as packaged by Arch does not emulate ESP32.** Verified:

```bash
qemu-system-xtensa -M help     # only dc232b boards (kc705, lx200, ml605)
qemu-system-riscv32 -M help    # virt only, no esp32c3
```

So `qemu-system-xtensa -M esp32` fails here regardless of firmware. You need
Espressif's QEMU fork, or ESP-IDF's `idf.py qemu` wrapper.

## Option A — ESP-IDF's QEMU (recommended)

```bash
git clone --depth 1 https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf && ./install.sh esp32,esp32c3
. ~/esp/esp-idf/export.sh
```

`idf.py qemu` builds the project, picks the right machine and flash image:

```bash
cp -r examples/get-started/blink ~/Projects/embedded/esp32/blink
cd ~/Projects/embedded/esp32/blink
idf.py set-target esp32
idf.py qemu            # or: idf.py qemu --gdb
```

## Option B — build Espressif's QEMU directly

```bash
git clone --depth 1 -b esp-develop https://github.com/espressif/qemu.git ~/esp/qemu
cd ~/esp/qemu
./configure --target-list=xtensa-softmmu,riscv32-softmmu --disable-werror
ninja -C build
build/qemu-system-xtensa -M help | grep esp
```

Then:

```bash
~/esp/qemu/build/qemu-system-xtensa -M esp32 -nographic \
  -drive file=build/flash_image.bin,if=mtd,format=raw
~/esp/qemu/build/qemu-system-riscv32 -M esp32c3 -nographic \
  -drive file=build/flash_image.bin,if=mtd,format=raw
```

## Firmware toolchain

Not installed by `setup/install.sh` (multi-GB). ESP-IDF's `install.sh` adds
`xtensa-esp32-elf-*` and `riscv32-esp-elf-*`. Plain Clang cannot produce
usable ESP32 Xtensa code without that backend setup.

## Debugging and traces

```bash
qemu-system-xtensa -M esp32 -S -gdb tcp::1234 ...
xtensa-esp32-elf-gdb build/blink.elf -ex 'target remote localhost:1234'
```

No VCD from QEMU; use `-trace <event> -D trace.log` or `-d in_asm -D asm.log`.
