// Running benchmarks (docs/spec.md §3, §4, §7): with and without measure(), in check mode, and failures.
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include "support/fakes.hpp"

using namespace std::chrono_literals;
namespace bench = cpm::bench;
using support::FakeClock;

namespace {

bench::Settings settings(bench::Mode mode = bench::Mode::measure) {
    bench::Settings result;
    result.mode = mode;
    result.samples = 5;
    result.sample_time = 10us;
    return result;
}

}  // namespace

TEST_CASE("a body without measure() is measured itself, after a run that warms up") {
    FakeClock clock;
    int runs = 0;
    const bench::Result result = support::run_one(
        [&] {
            ++runs;
            clock.advance(3us);
        },
        clock, settings());
    REQUIRE(result.measurement.has_value());
    CHECK(result.measurement->runs_per_sample == 4);  // 1, 2 and 4 runs to calibrate: 4 take 12us
    CHECK(result.measurement->samples == 5);
    CHECK(result.measurement->time.median == 3000.0);
    CHECK(result.measurement->time.spread == 0.0);
    // The first run, 1 + 2 + 4 to calibrate, 4 to warm up, and 5 samples of 4.
    CHECK(runs == 1 + 7 + 4 + 5 * 4);
    CHECK_FALSE(result.failed());
}

TEST_CASE("with measure(), the body's setup runs once and only the function is measured") {
    FakeClock clock;
    int setups = 0;
    int teardowns = 0;
    int runs = 0;
    const bench::Result result = support::run_one(
        [&] {
            ++setups;
            clock.advance(1s);  // setup isn't timed
            bench::measure([&] {
                ++runs;
                clock.advance(250ns);
            });
            ++teardowns;
        },
        clock, settings());
    REQUIRE(result.measurement.has_value());
    CHECK(setups == 1);
    CHECK(teardowns == 1);
    CHECK(result.measurement->time.median == 250.0);
    CHECK(result.measurement->runs_per_sample == 64);  // 64 runs take 16us
    CHECK(runs == (1 + 2 + 4 + 8 + 16 + 32 + 64) + 64 + 5 * 64);
}

TEST_CASE("measure() keeps what the function returns") {
    FakeClock clock;
    int runs = 0;
    const bench::Result result = support::run_one(
        [&] {
            bench::measure([&] {
                clock.advance(1us);
                return ++runs;
            });
        },
        clock, settings(bench::Mode::check));
    CHECK(runs == 1);
    CHECK_FALSE(result.failed());
}

TEST_CASE("items() and bytes() give throughput") {
    FakeClock clock;
    const bench::Result result = support::run_one(
        [&] {
            bench::items(1000);
            bench::bytes(4000);
            bench::measure([&] { clock.advance(2us); });
        },
        clock, settings());
    CHECK(result.items == std::optional<std::uint64_t>(1000));
    CHECK(result.bytes == std::optional<std::uint64_t>(4000));
    REQUIRE(result.measurement.has_value());
    CHECK(bench::runs_per_second(*result.measurement) == std::optional<double>(500'000.0));
    CHECK(bench::items_per_second(result) == std::optional<double>(500'000'000.0));
    CHECK(bench::bytes_per_second(result) == std::optional<double>(2'000'000'000.0));
}

TEST_CASE("without items() or bytes() there is no throughput") {
    FakeClock clock;
    const bench::Result result = support::run_one([&] { clock.advance(1us); }, clock, settings());
    CHECK(result.items == std::nullopt);
    CHECK(bench::items_per_second(result) == std::nullopt);
    CHECK(bench::bytes_per_second(result) == std::nullopt);
}

TEST_CASE("check mode runs each benchmark's code once, unmeasured") {
    FakeClock clock;
    int body_runs = 0;
    const bench::Result whole = support::run_one([&] { ++body_runs; }, clock, settings(bench::Mode::check));
    CHECK(body_runs == 1);
    CHECK(whole.measurement == std::nullopt);
    CHECK_FALSE(whole.failed());

    int setups = 0;
    int function_runs = 0;
    const bench::Result measured = support::run_one(
        [&] {
            ++setups;
            bench::items(10);
            bench::measure([&] { ++function_runs; });
        },
        clock, settings(bench::Mode::check));
    CHECK(setups == 1);
    CHECK(function_runs == 1);
    CHECK(measured.measurement == std::nullopt);
    CHECK(measured.items == std::optional<std::uint64_t>(10));
}

TEST_CASE("a benchmark that throws fails with the message, and the others still run") {
    FakeClock clock;
    bench::Registry registry;
    registry.add(support::benchmark("first", [&] { clock.advance(1us); }));
    registry.add(support::benchmark("throws", [] { throw std::runtime_error("out of range"); }));
    int calls = 0;
    registry.add(support::benchmark("throws later", [&] {
        bench::measure([&] {
            clock.advance(1us);
            if (++calls == 20) throw std::runtime_error("ran out");
        });
    }));
    registry.add(support::benchmark("last", [&] { clock.advance(1us); }));
    support::RecordingReporter reporter;
    const std::vector<bench::Result> results = bench::run(support::all(registry), settings(), clock, reporter);
    REQUIRE(results.size() == 4);
    CHECK_FALSE(results[0].failed());
    CHECK(results[1].error == std::optional<std::string>("out of range"));
    CHECK(results[1].measurement == std::nullopt);
    CHECK(results[2].error == std::optional<std::string>("ran out"));
    CHECK_FALSE(results[3].failed());
    CHECK(results[3].measurement.has_value());
}

TEST_CASE("an exception of another type fails too") {
    FakeClock clock;
    const bench::Result result = support::run_one([] { throw 42; }, clock, settings());
    CHECK(result.error == std::optional<std::string>("an exception that isn't a std::exception"));
}

TEST_CASE("measure() called twice fails the benchmark") {
    FakeClock clock;
    for (const bench::Mode mode : {bench::Mode::measure, bench::Mode::check}) {
        const bench::Result result = support::run_one(
            [&] {
                bench::measure([&] { clock.advance(1ms); });
                bench::measure([&] { clock.advance(1ms); });
            },
            clock, settings(mode));
        CHECK(result.error == std::optional<std::string>("measure() was called twice in one benchmark"));
    }
}

TEST_CASE("measure() on a later run of a body, but not the first, fails the benchmark") {
    FakeClock clock;
    int runs = 0;
    const bench::Result result = support::run_one(
        [&] {
            clock.advance(1ms);
            if (++runs == 3) bench::measure([] {});
        },
        clock, settings());
    CHECK(result.error ==
          std::optional<std::string>("measure() was called on a later run of the body, but not on the first"));
}

TEST_CASE("measure(), items() and bytes() outside a benchmark throw std::logic_error") {
    CHECK_THROWS_AS(bench::measure([] {}), std::logic_error);
    CHECK_THROWS_WITH(bench::measure([] {}), "measure() was called outside a benchmark");
    CHECK_THROWS_WITH(bench::items(1), "items() was called outside a benchmark");
    CHECK_THROWS_WITH(bench::bytes(1), "bytes() was called outside a benchmark");
}

TEST_CASE("after a benchmark, measure() is outside one again") {
    FakeClock clock;
    support::run_one([&] { clock.advance(1ms); }, clock, settings(bench::Mode::check));
    support::run_one([] { throw std::runtime_error("x"); }, clock, settings(bench::Mode::check));
    CHECK_THROWS_AS(bench::measure([] {}), std::logic_error);
}

TEST_CASE("the reporter hears of each benchmark in order") {
    FakeClock clock;
    bench::Registry registry;
    registry.add(support::benchmark("a", [&] { clock.advance(1ms); }));
    registry.add(support::benchmark("b", [] { throw std::runtime_error("no"); }));
    support::RecordingReporter reporter;
    bench::run(support::all(registry), settings(), clock, reporter);
    CHECK(reporter.events == std::vector<std::string>{"started 2", "finished a", "finished b", "ended 2"});
}

TEST_CASE("results carry the benchmark's name and build") {
    FakeClock clock;
    bench::Registry registry;
    registry.add(support::benchmark("debug", [&] { clock.advance(1ms); }, bench::Build{false, false}));
    support::RecordingReporter reporter;
    const std::vector<bench::Result> results = bench::run(support::all(registry), settings(), clock, reporter);
    CHECK(results.at(0).name == "debug");
    CHECK_FALSE(results.at(0).build.optimized);
    CHECK_FALSE(results.at(0).build.release());
}

TEST_CASE("a measurement records the clock's resolution") {
    FakeClock clock(40ns);
    const bench::Result result = support::run_one([&] { clock.advance(1ms); }, clock, settings());
    REQUIRE(result.measurement.has_value());
    CHECK(result.measurement->resolution == 40ns);
    CHECK_FALSE(bench::too_fast(*result.measurement));
}
