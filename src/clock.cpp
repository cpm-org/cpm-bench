#include <cpm/bench/clock.hpp>

#include <algorithm>

namespace cpm::bench {

std::chrono::nanoseconds SteadyClock::now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

std::chrono::nanoseconds SteadyClock::resolution() const {
    const auto tick = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::duration(1));
    return std::max(tick, std::chrono::nanoseconds(1));
}

}  // namespace cpm::bench
