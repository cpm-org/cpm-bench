#include "calibration.hpp"

namespace cpm::bench::detail {

std::uint64_t calibrate(const Batch& batch, Clock& clock, std::chrono::nanoseconds sample_time) {
    std::uint64_t runs = 1;
    while (true) {
        const std::chrono::nanoseconds start = clock.now();
        batch(runs);
        if (clock.now() - start >= sample_time || runs >= largest_batch) return runs;
        runs *= 2;
    }
}

}  // namespace cpm::bench::detail
