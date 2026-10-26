#include <cpm/bench/result.hpp>

namespace cpm::bench {
namespace {

constexpr double noisy_spread = 5.0;     // percent
constexpr double too_fast_ticks = 10.0;  // of the clock

std::optional<double> per_second(double median, std::uint64_t count) noexcept {
    if (!(median > 0)) return std::nullopt;
    return static_cast<double>(count) * 1e9 / median;
}

std::optional<double> per_second(const Result& result, std::optional<std::uint64_t> count) noexcept {
    if (!result.measurement || !count) return std::nullopt;
    return per_second(result.measurement->time.median, *count);
}

}  // namespace

bool noisy(const Measurement& measurement) noexcept { return measurement.time.spread > noisy_spread; }

bool too_fast(const Measurement& measurement) noexcept {
    return measurement.time.median < too_fast_ticks * static_cast<double>(measurement.resolution.count());
}

std::optional<double> runs_per_second(const Measurement& measurement) noexcept {
    return per_second(measurement.time.median, 1);
}

std::optional<double> items_per_second(const Result& result) noexcept { return per_second(result, result.items); }

std::optional<double> bytes_per_second(const Result& result) noexcept { return per_second(result, result.bytes); }

}  // namespace cpm::bench
