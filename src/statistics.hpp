// Summarizing the samples (docs/spec.md §4, step 4).
#pragma once

#include <cpm/bench/result.hpp>

#include <vector>

namespace cpm::bench::detail {

// The middle value once sorted, or the mean of the two middle ones; 0 for none.
double median(std::vector<double> values);

// The median, extremes and spread of `times`. All 0 for no times.
Statistics summarize(const std::vector<double>& times);

}  // namespace cpm::bench::detail
