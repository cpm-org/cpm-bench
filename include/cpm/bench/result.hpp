// What running a benchmark gives (docs/spec.md §4): its measurement, or why it failed.
#pragma once

#include <cpm/bench/registry.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace cpm::bench {

// The time per run over a benchmark's samples, in nanoseconds.
struct Statistics {
    double median = 0;
    double min = 0;
    double max = 0;
    double spread = 0;  // the median absolute deviation from the median, as a percentage of the median
};

struct Measurement {
    Statistics time;
    std::size_t samples = 0;
    std::uint64_t runs_per_sample = 0;
    std::chrono::nanoseconds resolution{1};  // the clock's
};

struct Result {
    std::string name;
    Build build;
    std::optional<std::uint64_t> items;      // per run, when the benchmark said
    std::optional<std::uint64_t> bytes;      // per run, when the benchmark said
    std::optional<Measurement> measurement;  // none in check mode, or when the benchmark failed
    std::optional<std::string> error;        // why the benchmark failed

    bool failed() const noexcept { return error.has_value(); }
};

// Whether the spread is above 5%: something else may have been running.
bool noisy(const Measurement& measurement) noexcept;

// Whether a run takes less than 10 times the clock's resolution, so loop and call overhead are a large part of it.
bool too_fast(const Measurement& measurement) noexcept;

// Runs, items and bytes per second, from the median. nullopt when there is nothing to divide by.
std::optional<double> runs_per_second(const Measurement& measurement) noexcept;
std::optional<double> items_per_second(const Result& result) noexcept;
std::optional<double> bytes_per_second(const Result& result) noexcept;

}  // namespace cpm::bench
