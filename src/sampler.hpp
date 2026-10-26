// The warm-up and the samples (docs/spec.md §4, steps 2 and 3).
#pragma once

#include <cpm/bench/clock.hpp>
#include <cpm/bench/measure.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cpm::bench::detail {

// Runs one batch of `runs` runs to warm up, without timing it, then times `samples` batches. Returns each sample's
// time per run, in nanoseconds.
std::vector<double> sample(const Batch& batch, Clock& clock, std::uint64_t runs, std::size_t samples);

}  // namespace cpm::bench::detail
