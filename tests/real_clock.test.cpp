// The real clock (docs/spec.md §4): a short measurement of a real loop gives positive, consistent numbers.
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

#include "support/fakes.hpp"

using namespace std::chrono_literals;
namespace bench = cpm::bench;

TEST_CASE("the steady clock goes forward, with a tick of at least 1 ns") {
    bench::SteadyClock clock;
    const auto first = clock.now();
    const auto second = clock.now();
    CHECK(second >= first);
    CHECK(clock.resolution() >= 1ns);
    CHECK(&bench::steady_clock() == &bench::steady_clock());
}

TEST_CASE("a real loop measures with positive, consistent numbers") {
    bench::Settings settings;
    settings.samples = 5;
    settings.sample_time = 1ms;
    std::vector<std::uint64_t> data(1000);
    for (std::uint64_t i = 0; i < data.size(); ++i) data[i] = i * 7 % 13;
    const bench::Result result = support::run_one(
        [&] {
            bench::items(data.size());
            bench::measure([&] {
                std::uint64_t sum = 0;
                for (const std::uint64_t value : data) {
                    sum += value;
                    bench::keep(sum);
                }
                return sum;
            });
        },
        bench::steady_clock(), settings);
    REQUIRE_FALSE(result.failed());
    REQUIRE(result.measurement.has_value());
    const bench::Measurement& measurement = *result.measurement;
    CHECK(measurement.samples == 5);
    CHECK(measurement.runs_per_sample >= 1);
    CHECK(measurement.time.median > 0.0);
    CHECK(measurement.time.min <= measurement.time.median);
    CHECK(measurement.time.median <= measurement.time.max);
    CHECK(measurement.time.spread >= 0.0);
    // A batch takes at least the sample time, so the median run is at least the sample time over the batch size.
    CHECK(measurement.time.median * static_cast<double>(measurement.runs_per_sample) >= 1e6 * 0.5);
    CHECK(bench::items_per_second(result).value_or(0) > 0.0);
}
