#include "sampler.hpp"

#include <chrono>

namespace cpm::bench::detail {

std::vector<double> sample(const Batch& batch, Clock& clock, std::uint64_t runs, std::size_t samples) {
    batch(runs);
    std::vector<double> times;
    times.reserve(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const std::chrono::nanoseconds start = clock.now();
        batch(runs);
        const std::chrono::nanoseconds elapsed = clock.now() - start;
        times.push_back(static_cast<double>(elapsed.count()) / static_cast<double>(runs));
    }
    return times;
}

}  // namespace cpm::bench::detail
