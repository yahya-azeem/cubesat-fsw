# CSPICE wrapper (C++23)

`Cubesat_Spice` is a bounded, fail-closed C++23 wrapper around NAIF CSPICE.
It is what flight code should call; raw `spice_*_c` calls do not belong in a
rate group.

## Safety properties

| Concern | Handling |
|---|---|
| CSPICE aborts on error | `erract_c("SET", …, "RETURN")`, `errprt_c(..., "NONE")` |
| Global error state | checked after every call, `reset_c()` on failure |
| Fixed C buffers | all inputs copied into bounded arrays; overlong strings rejected |
| Embedded NUL / empty strings | rejected with `Code::InvalidArgument` |
| `double` inputs | NaN/Inf rejected before calling CSPICE |
| State output | position, velocity and light time must be finite |
| Frame matrices | orthonormality and `det = +1` enforced before returning |
| Kernel lifetime | RAII `Kernel`, move-only, unloaded on destruction |
| Exceptions / allocation | none; compiled `-fno-exceptions -fno-rtti` |

Errors are values: `std::expected<T, Error>` (C++23). There is no `last_error`
global to race on.

## API

```cpp
namespace cubesat::spice {

Result<Kernel>?             // Kernel::load(path) -> Result<void>
Result<double>              utc_to_et(std::string_view utc);
Result<Text<40>>            et_to_utc(double et, UtcFormat = Isoc, int precision = 3);
Result<State>               state(target, et, frame="J2000",
                                  abcorr="NONE", observer="EARTH");
Result<Matrix3>             frame_transform(from, to, et);
}
```

`Error` carries `Code` (`InvalidArgument`, `KernelLoad`, `TimeConversion`,
`Ephemeris`, `Frame`, `NonFinite`, `SpiceFailure`) plus a bounded message.

Example:

```cpp
cubesat::spice::Kernel lsk;
if (auto r = lsk.load("kernels/naif0012.tls"); !r) {
    // r.error().code / r.error().message
}

auto et = cubesat::spice::utc_to_et("2026-11-12T00:00:00");
if (!et) { /* fail closed */ }

auto s = cubesat::spice::state("-999", *et, "J2000", "NONE", "EARTH");
if (s) {
    // s->position_km, s->velocity_km_s, s->light_time_s
}
```

## Kernels

Vendored under `Cubesat/Spice/kernels/`:

| Kernel | Purpose | Size |
|---|---|---|
| `naif0012.tls` | leap seconds (UTC ↔ ET) | 5 KB |
| `pck00011.tpc` | body orientation constants (`IAU_EARTH`, …) | 129 KB |

Ephemeris files (SPKs) are mission-specific and not committed. The tests
write a small synthetic type-8 SPK at runtime, so no 17 MB `de421.bsp` is
needed. For real work, furnish a mission SPK the same way.

CSPICE is not thread-safe. Load kernels during initialization and call the
wrapper from one thread (the rate group). The wrapper deliberately does not
add a lock: a spinlock in an interrupt context is worse than a documented
single-thread contract.

## Build

```bash
./setup/install-cspice.sh          # downloads libcspice.a + headers
make fsw                           # runs test_spice with the kernels
```

`CSPICE_ROOT` can point at a system or custom toolkit:

```bash
cmake -DCSPICE_ROOT=/opt/cspice ...
```

The target is C++23 (`target_compile_features(... cxx_std_23)`); components
that include `spice.hpp` are compiled as C++23 too. The rest of F´ stays on
the project's C++20 setting.

## Tests

`fsw/tests/test_spice.cpp` covers:

- J2000 epoch, UTC→ET→UTC round trip, C/D/ISOC/ISOD formats
- invalid dates, empty/overlong/NUL strings, bad precision, NaN/Inf ET
- missing/overlong kernels, kernel move, unload and reload
- synthetic SPK state lookup, out-of-coverage and unknown-body failures
- error recovery: a failed call does not poison the next one
- identity and `IAU_EARTH` transforms, determinant and inverse check

Run under ASan/UBSan:

```bash
g++ -std=c++23 -fsanitize=address,undefined ... test_spice.cpp spice.cpp \
    ../third_party/cspice/lib/cspice.a -lm
```
