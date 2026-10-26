// Where measurements get the time (docs/spec.md §4). The runner reads a steady clock; tests use a clock that advances
// exactly as they say.
#pragma once

#include <chrono>

namespace cpm::bench {

class Clock {
public:
    virtual ~Clock() = default;

    // The time since an arbitrary start, which never goes back.
    virtual std::chrono::nanoseconds now() = 0;

    // The clock's tick: the smallest step between two readings.
    virtual std::chrono::nanoseconds resolution() const = 0;

protected:
    Clock() = default;
    Clock(const Clock&) = default;
    Clock& operator=(const Clock&) = default;
};

// std::chrono::steady_clock.
class SteadyClock final : public Clock {
public:
    std::chrono::nanoseconds now() override;
    std::chrono::nanoseconds resolution() const override;
};

}  // namespace cpm::bench
