// How many runs a sample takes (docs/spec.md §4, step 1).
#pragma once

#include <cpm/bench/clock.hpp>
#include <cpm/bench/measure.hpp>

#include <chrono>
#include <cstdint>

namespace cpm::bench::detail {

// Code the compiler removed takes no time however many runs a batch has, so doubling stops here.
inline constexpr std::uint64_t largest_batch = std::uint64_t{1} << 40;

// The first of 1, 2, 4, ... runs whose batch takes at least `sample_time` (or largest_batch).
std::uint64_t calibrate(const Batch& batch, Clock& clock, std::chrono::nanoseconds sample_time);

}  // namespace cpm::bench::detail
