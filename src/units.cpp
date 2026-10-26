#include "units.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <span>
#include <string_view>

namespace cpm::bench::detail {
namespace {

// The power of ten of `value`'s first digit once rounded to four significant digits: 9.9996 is 1, since it shows as
// 10.00.
int magnitude(double value) {
    const int first = static_cast<int>(std::floor(std::log10(value)));
    return std::round(value * std::pow(10.0, 3 - first)) >= 10000 ? first + 1 : first;
}

// `value` in the first of `units` that keeps it below 1000, or in the last.
std::string with_unit(double value, std::span<const std::string_view> units) {
    std::size_t unit = 0;
    while (unit + 1 < units.size() && value > 0 && magnitude(value) >= 3) {
        value /= 1000;
        ++unit;
    }
    return significant(value) + std::string(units[unit]);
}

constexpr std::array<std::string_view, 4> time_units{" ns", " us", " ms", " s"};
constexpr std::array<std::string_view, 4> count_units{"", "k", "M", "G"};
constexpr std::array<std::string_view, 4> byte_rate_units{" B/s", " KB/s", " MB/s", " GB/s"};

}  // namespace

std::string significant(double value) {
    if (!(value > 0)) return "0";
    return std::format("{:.{}f}", value, std::max(0, 3 - magnitude(value)));
}

std::string format_time(double nanoseconds) { return with_unit(nanoseconds, time_units); }

std::string format_count(double count) { return with_unit(count, count_units); }

std::string format_byte_rate(double bytes_per_second) { return with_unit(bytes_per_second, byte_rate_units); }

std::string format_percent(double percent) { return std::format("{:.1f}%", percent); }

}  // namespace cpm::bench::detail
