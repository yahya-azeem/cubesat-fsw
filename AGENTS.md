# Engineering policy

## Flight software: F´ (C++)

The mission FSW is **NASA F Prime**, C++ on FreeRTOS. The F´ autocoder owns
the topology; hand-written code is limited to components and platform glue.

Rust is **not** used for flight components. It is supported in exactly one
place: `Svc::WasmSequencer` sequences (Rust → Wasm, isolated, uploaded at
runtime). Keep that boundary sharp.

## Language rules

### C++ (flight components, host tools)

- C++20, restricted subset. Build with
  `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`.
- No heap after initialization. No `new`/`delete`, no `std::string`,
  `std::vector`, `std::map`, streams or `iostream` in flight code.
- Prefer `std::array`, `std::span`, `std::optional`, `constexpr`,
  `enum class`, `[[nodiscard]]`, `static_assert`.
- No virtual dispatch in a rate-group path. No singletons. No hidden
  globals; state lives in the component instance.
- Errors travel through F´ ports/events, not exceptions. Every `noexcept`
  function that can fail returns a status or `std::optional`.
- Prefer passive components. Active components (their own thread) need a
  written justification and a measured stack budget.
- Zero-copy: payload data moves as `Svc::BufferManager` buffers, never by
  copying into a component-owned array.

### C (AVR demo, vendor BSPs, C libraries behind FFI)

- C17, no compiler extensions unless the vendor header requires them.
- Fixed-width types (`stdint.h`), `const`-correct pointers, `static` for
  internal linkage, explicit casts on every narrowing conversion.
- No VLAs, no `alloca`, no unbounded `strcpy`/`sprintf`. Use
  `snprintf` and check the result; pass buffer length with every pointer.
- No dynamic allocation after startup. If a vendor API forces it, wrap it
  in one audited module and document the bound.
- `main` returns `int`; functions taking nothing use `(void)`.
- Treat warnings as errors: `-std=c17 -Wall -Wextra -Wpedantic -Wshadow
  -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes
  -Wvla -Wundef -Werror`.

## Anti-bloat

- No framework per subsystem, no Boost, no template metaprogramming beyond
  what a component actually needs.
- One dependency requires a written reason in the commit message and a
  check that it fits the flash/RAM budget.
- Prefer the F´ autocoder over hand-rolled message plumbing.
- Static allocation, bounded queues, fixed-size buffers. If a number is not
  in a config header, it is not a number.

## Memory-determinism checklist (per component)

1. Everything allocated at init (`Fw::MemAllocator` / buffer pool).
2. Queue depth and buffer size chosen from the worst-case pass, not a guess.
3. No recursion.
4. Worst-case stack measured, not estimated.
5. Fault path is fail-closed: PA off, payload off, beacon only.

## Testing

- Every pure-logic module gets host unit tests that run without hardware:
  `make -C fsw` (or `make fsw` from the root).
- Every F´ component gets a `register_fprime_ut` harness and must pass
  `make fprime-ut` (`fprime-util check`).
- Cover boundaries, failure paths and a full mission-duration scenario, not
  just the happy path. Fault and corruption paths are mandatory.
- A new component without tests is not done. Tests must not need the F´
  runtime, a serial port or a radio.
- `make fprime` must stay green after component changes; a component that
  only passes its own test but breaks the topology is not integrated.

## Layout

| Path | Language | Role |
|---|---|---|
| `fsw/fprime/cubesat-fsw/Cubesat/PowerCore` | C++20 | battery, mode FSM, CCSDS |
| `fsw/fprime/cubesat-fsw/Cubesat/Storage` | C++20 + C17 | LittleFS wrapper |
| `fsw/fprime/cubesat-fsw/Cubesat/Components` | C++20 | F´ passive components |
| `fsw/fprime/cubesat-fsw/CubesatDeployment` | FPP | topology and instances |
| `fsw/tests/` | C++20 | host unit tests |
| `avr/`, `riscv/` | C17 | board/emulator demos |
| `rust/` | Rust | host-side reference models and QEMU sandbox |
| `tools/` | shell | PAC generation, helpers |
