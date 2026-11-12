#include "spice.hpp"

#include <SpiceUsr.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace cubesat::spice {

namespace {

constexpr double kRotationTolerance = 1e-9;

void set_text(Text<kMaxError>& out, std::string_view text) noexcept
{
    const std::size_t length = std::min(text.size(), out.data.size() - 1);
    if (length > 0) {
        std::copy_n(text.data(), length, out.data.data());
    }
    out.data[length] = '\0';
    out.length = length;
}

Error make_error(Code code, std::string_view message) noexcept
{
    Error error{};
    error.code = code;
    set_text(error.message, message);
    return error;
}

std::unexpected<Error> fail(Code code, std::string_view message) noexcept
{
    return std::unexpected<Error>(make_error(code, message));
}

std::size_t bounded_length(const char* text, std::size_t capacity) noexcept
{
    std::size_t length = 0;
    while (length < capacity && text[length] != '\0') {
        ++length;
    }
    return length;
}

/// Error from the most recent failed CSPICE call; resets CSPICE so the next
/// call starts clean (required with erract = RETURN).
Error take_spice_error(Code code, std::string_view fallback) noexcept
{
    Error error = make_error(code, fallback);
    if (failed_c() == 0) {
        return error;
    }

    std::array<SpiceChar, kMaxError> buffer{};
    getmsg_c("SHORT", static_cast<SpiceInt>(buffer.size()), buffer.data());
    set_text(error.message,
             std::string_view(buffer.data(), bounded_length(buffer.data(), buffer.size())));
    reset_c();
    return error;
}

std::optional<Error> check_text(std::string_view value, std::size_t capacity,
                                std::string_view what) noexcept
{
    if (value.empty()) {
        return make_error(Code::InvalidArgument, what);
    }
    if (value.size() >= capacity) {
        return make_error(Code::InvalidArgument, "argument exceeds buffer");
    }
    if (value.find('\0') != std::string_view::npos) {
        return make_error(Code::InvalidArgument, "embedded NUL in argument");
    }
    return std::nullopt;
}

bool copy_string(std::string_view value, char* out, std::size_t capacity) noexcept
{
    if (value.size() >= capacity) {
        return false;
    }
    if (!value.empty()) {
        std::copy_n(value.data(), value.size(), out);
    }
    out[value.size()] = '\0';
    return true;
}

bool finite_state(const State& state) noexcept
{
    for (const double value : state.position_km) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    for (const double value : state.velocity_km_s) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return std::isfinite(state.light_time_s);
}

double dot(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

bool valid_rotation(const Matrix3& matrix) noexcept
{
    const auto& m = matrix.m;

    for (const auto& row : m) {
        if (std::abs(dot(row, row) - 1.0) > kRotationTolerance) {
            return false;
        }
    }
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = i + 1; j < 3; ++j) {
            if (std::abs(dot(m[i], m[j])) > kRotationTolerance) {
                return false;
            }
        }
    }

    const double det =
        (m[0][0] * ((m[1][1] * m[2][2]) - (m[1][2] * m[2][1]))) -
        (m[0][1] * ((m[1][0] * m[2][2]) - (m[1][2] * m[2][0]))) +
        (m[0][2] * ((m[1][0] * m[2][1]) - (m[1][1] * m[2][0])));

    return std::abs(det - 1.0) <= kRotationTolerance;
}

constexpr std::array<const char*, 4> kFormats{"ISOC", "ISOD", "C", "J"};

}  // namespace

void initialize() noexcept
{
    // Function-local static: thread-safe once-initialization (C++11).
    static const bool ready = [] {
        SpiceChar action[] = "RETURN";
        SpiceChar list[] = "NONE";
        erract_c("SET", static_cast<SpiceInt>(std::strlen(action)), action);
        errprt_c("SET", static_cast<SpiceInt>(std::strlen(list)), list);
        reset_c();
        return true;
    }();
    static_cast<void>(ready);
}

// ---------------------------------------------------------------- kernels --

Kernel::~Kernel() { static_cast<void>(unload()); }

Kernel::Kernel(Kernel&& other) noexcept : path_(other.path_), loaded_(other.loaded_)
{
    other.loaded_ = false;
}

Kernel& Kernel::operator=(Kernel&& other) noexcept
{
    if (this != &other) {
        static_cast<void>(unload());
        path_ = other.path_;
        loaded_ = other.loaded_;
        other.loaded_ = false;
    }
    return *this;
}

Result<void> Kernel::load(std::string_view path) noexcept
{
    initialize();

    if (const auto problem = check_text(path, kMaxPath, "empty kernel path")) {
        return std::unexpected<Error>(*problem);
    }

    if (loaded_) {
        auto unloaded = unload();
        if (!unloaded) {
            return unloaded;
        }
    }

    copy_string(path, path_.data(), path_.size());

    furnsh_c(path_.data());
    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::KernelLoad, "kernel furnishing failed"));
    }

    loaded_ = true;
    return {};
}

Result<void> Kernel::unload() noexcept
{
    if (!loaded_) {
        return {};
    }

    unload_c(path_.data());
    loaded_ = false;
    path_.fill('\0');

    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::KernelLoad, "kernel unload failed"));
    }
    return {};
}

// ------------------------------------------------------------ conversions --

Result<double> utc_to_et(std::string_view utc) noexcept
{
    initialize();

    if (const auto problem = check_text(utc, kMaxTime, "empty UTC string")) {
        return std::unexpected<Error>(*problem);
    }

    std::array<SpiceChar, kMaxTime> buffer{};
    copy_string(utc, buffer.data(), buffer.size());

    SpiceDouble et = 0.0;
    str2et_c(buffer.data(), &et);
    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::TimeConversion, "UTC to ET conversion failed"));
    }
    if (!std::isfinite(et)) {
        return fail(Code::NonFinite, "conversion produced a non-finite ET");
    }
    return et;
}

Result<Text<kMaxUtc>> et_to_utc(double et, UtcFormat format, int precision) noexcept
{
    initialize();

    if (!std::isfinite(et)) {
        return fail(Code::NonFinite, "ET is not finite");
    }
    if (precision < 0 || precision > 9) {
        return fail(Code::InvalidArgument, "precision must be 0..9");
    }

    const auto index = static_cast<std::size_t>(format);
    if (index >= kFormats.size()) {
        return fail(Code::InvalidArgument, "unknown UTC format");
    }

    std::array<SpiceChar, kMaxUtc> buffer{};
    et2utc_c(et, kFormats[index], static_cast<SpiceInt>(precision),
             static_cast<SpiceInt>(buffer.size()), buffer.data());
    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::TimeConversion, "ET to UTC conversion failed"));
    }

    Text<kMaxUtc> out{};
    const std::size_t length = bounded_length(buffer.data(), buffer.size());
    std::copy_n(buffer.data(), length, out.data.data());
    out.data[length] = '\0';
    out.length = length;
    return out;
}

Result<State> state(std::string_view target, double et, std::string_view frame,
                    std::string_view abcorr, std::string_view observer) noexcept
{
    initialize();

    if (!std::isfinite(et)) {
        return fail(Code::NonFinite, "ET is not finite");
    }
    for (const auto& [value, what] :
         {std::pair{target, "empty target"},
          std::pair{frame, "empty frame"},
          std::pair{abcorr, "empty aberration"},
          std::pair{observer, "empty observer"}}) {
        if (const auto problem = check_text(value, kMaxName, what)) {
            return std::unexpected<Error>(*problem);
        }
    }

    std::array<SpiceChar, kMaxName> target_b{};
    std::array<SpiceChar, kMaxName> frame_b{};
    std::array<SpiceChar, kMaxName> abcorr_b{};
    std::array<SpiceChar, kMaxName> observer_b{};
    copy_string(target, target_b.data(), target_b.size());
    copy_string(frame, frame_b.data(), frame_b.size());
    copy_string(abcorr, abcorr_b.data(), abcorr_b.size());
    copy_string(observer, observer_b.data(), observer_b.size());

    SpiceDouble raw[6] = {0, 0, 0, 0, 0, 0};
    SpiceDouble light_time = 0.0;
    spkezr_c(target_b.data(), et, frame_b.data(), abcorr_b.data(),
             observer_b.data(), raw, &light_time);
    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::Ephemeris, "state lookup failed"));
    }

    State out{};
    for (std::size_t i = 0; i < 3; ++i) {
        out.position_km[i] = raw[i];
        out.velocity_km_s[i] = raw[i + 3];
    }
    out.light_time_s = light_time;

    if (!finite_state(out)) {
        return fail(Code::NonFinite, "state contains non-finite values");
    }
    return out;
}

Result<Matrix3> frame_transform(std::string_view from, std::string_view to,
                                double et) noexcept
{
    initialize();

    if (!std::isfinite(et)) {
        return fail(Code::NonFinite, "ET is not finite");
    }
    if (const auto problem = check_text(from, kMaxName, "empty source frame")) {
        return std::unexpected<Error>(*problem);
    }
    if (const auto problem = check_text(to, kMaxName, "empty destination frame")) {
        return std::unexpected<Error>(*problem);
    }

    std::array<SpiceChar, kMaxName> from_b{};
    std::array<SpiceChar, kMaxName> to_b{};
    copy_string(from, from_b.data(), from_b.size());
    copy_string(to, to_b.data(), to_b.size());

    SpiceDouble raw[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    pxform_c(from_b.data(), to_b.data(), et, raw);
    if (failed_c() != 0) {
        return std::unexpected<Error>(
            take_spice_error(Code::Frame, "frame transform failed"));
    }

    Matrix3 out{};
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            out.m[row][column] = raw[row][column];
        }
    }

    if (!valid_rotation(out)) {
        return fail(Code::Frame, "frame matrix is not orthonormal");
    }
    return out;
}

}  // namespace cubesat::spice
