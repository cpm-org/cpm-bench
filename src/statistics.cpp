#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace cpm::bench::detail {

double median(std::vector<double> values) {
    if (values.empty()) return 0;
    std::ranges::sort(values);
    const std::size_t middle = values.size() / 2;
    return values.size() % 2 == 1 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
}

Statistics summarize(const std::vector<double>& times) {
    if (times.empty()) return {};
    Statistics statistics;
    statistics.median = median(times);
    const auto [min, max] = std::ranges::minmax_element(times);
    statistics.min = *min;
    statistics.max = *max;
    std::vector<double> deviations;
    deviations.reserve(times.size());
    for (const double time : times) deviations.push_back(std::abs(time - statistics.median));
    if (statistics.median > 0) statistics.spread = median(std::move(deviations)) / statistics.median * 100;
    return statistics;
}

}  // namespace cpm::bench::detail
