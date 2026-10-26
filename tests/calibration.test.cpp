// Calibration and sampling with a fake clock (docs/spec.md §4, steps 1 to 3).
#include <calibration.hpp>
#include <sampler.hpp>

#include <cpm/test.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

#include "support/fakes.hpp"

using namespace std::chrono_literals;
using cpm::bench::detail::calibrate;
using cpm::bench::detail::largest_batch;
using cpm::bench::detail::sample;
using support::FakeClock;

namespace {

// A batch whose runs each take `per_run` on `clock`, and that records the size of every batch.
struct Costly {
    FakeClock& clock;
    std::chrono::nanoseconds per_run;
    std::vector<std::uint64_t> batches;

    cpm::bench::detail::Batch batch() {
        return [this](std::uint64_t runs) {
            batches.push_back(runs);
            clock.advance(per_run * static_cast<std::int64_t>(runs));
        };
    }
};

}  // namespace

TEST_CASE("calibration doubles the batch until it takes the sample time") {
    FakeClock clock;
    Costly code{clock, 1us, {}};
    CHECK(calibrate(code.batch(), clock, 10us) == 16);
    CHECK(code.batches == std::vector<std::uint64_t>{1, 2, 4, 8, 16});
}

TEST_CASE("a batch that takes exactly the sample time is enough") {
    FakeClock clock;
    Costly code{clock, 5us, {}};
    CHECK(calibrate(code.batch(), clock, 10us) == 2);
    CHECK(code.batches == std::vector<std::uint64_t>{1, 2});
}

TEST_CASE("one run longer than the sample time is a batch of 1") {
    FakeClock clock;
    Costly code{clock, 25ms, {}};
    CHECK(calibrate(code.batch(), clock, 10ms) == 1);
    CHECK(code.batches == std::vector<std::uint64_t>{1});
}

TEST_CASE("code that takes no time stops doubling at the largest batch") {
    FakeClock clock;
    Costly code{clock, 0ns, {}};
    CHECK(calibrate(code.batch(), clock, 10ms) == largest_batch);
    CHECK(code.batches.size() == 41);
}

TEST_CASE("sampling warms up with one batch it doesn't record") {
    FakeClock clock;
    std::vector<std::uint64_t> batches;
    int calls = 0;
    // The warm-up batch is slow, as a cold cache would make it; the samples aren't.
    const cpm::bench::detail::Batch batch = [&](std::uint64_t runs) {
        batches.push_back(runs);
        clock.advance((calls++ == 0 ? 1000ns : 10ns) * static_cast<std::int64_t>(runs));
    };
    const std::vector<double> times = sample(batch, clock, 4, 3);
    CHECK(batches == std::vector<std::uint64_t>{4, 4, 4, 4});
    CHECK(times == std::vector<double>{10.0, 10.0, 10.0});
}

TEST_CASE("each sample's time is per run") {
    FakeClock clock;
    std::vector<std::chrono::nanoseconds> costs{0ns, 800ns, 1200ns, 1000ns};  // the batch times, warm-up first
    std::size_t next = 0;
    const cpm::bench::detail::Batch batch = [&](std::uint64_t) { clock.advance(costs.at(next++)); };
    CHECK(sample(batch, clock, 8, 3) == std::vector<double>{100.0, 150.0, 125.0});
}
