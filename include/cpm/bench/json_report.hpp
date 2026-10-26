// The JSON report (docs/spec.md §6.2), for tools that track results over time.
#pragma once

#include <cpm/bench/result.hpp>

#include <span>
#include <string>

namespace cpm::bench {

// The results as a JSON document, indented, ending in a line break.
std::string json_report(std::span<const Result> results);

}  // namespace cpm::bench
