// The console report (docs/spec.md §6.1), compared exactly: from results made by hand, for each warning, and from a
// run on a fake clock.
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "support/fakes.hpp"

using namespace std::chrono_literals;
namespace bench = cpm::bench;

namespace {

bench::Result measured(std::string name, double median, double spread = 0.5) {
    bench::Result result;
    result.name = std::move(name);
    bench::Measurement measurement;
    measurement.time = bench::Statistics{median, median * 0.99, median * 1.2, spread};
    measurement.samples = 11;
    measurement.runs_per_sample = 64;
    result.measurement = measurement;
    return result;
}

bench::Result failed(std::string name, std::string error) {
    bench::Result result;
    result.name = std::move(name);
    result.error = std::move(error);
    return result;
}

// What a ConsoleReporter writes for `results` in `mode`.
std::string report(const std::vector<bench::Result>& results, bench::Mode mode = bench::Mode::measure) {
    std::vector<bench::Benchmark> benchmarks;
    for (const bench::Result& result : results) benchmarks.push_back(support::benchmark(result.name, [] {}));
    std::vector<const bench::Benchmark*> pointers;
    for (const bench::Benchmark& benchmark : benchmarks) pointers.push_back(&benchmark);
    bench::Settings settings;
    settings.mode = mode;
    std::ostringstream out;
    bench::ConsoleReporter reporter(out);
    reporter.run_started(pointers, settings);
    for (const bench::Result& result : results) reporter.benchmark_finished(result);
    reporter.run_finished(results);
    return out.str();
}

}  // namespace

TEST_CASE("a table aligned to the longest name, with throughput where a benchmark has it") {
    bench::Result sort = measured("sort 10000", 412'000.0, 1.2);
    sort.items = 10'000;
    bench::Result parse = measured("parse 1 MB", 3.1e6, 0.4);
    parse.bytes = 1'000'000;
    CHECK(report({measured("vector push_back 1000", 1230.0, 0.8), sort, parse}) ==
          "benchmark               time/run     spread   runs/s     throughput\n"
          "vector push_back 1000   1.230 us     0.8%     813.0k\n"
          "sort 10000              412.0 us     1.2%     2.427k     24.27M items/s\n"
          "parse 1 MB              3.100 ms     0.4%     322.6      322.6 MB/s\n");
}

TEST_CASE("a short name still leaves room for the header") {
    CHECK(report({measured("pi", 50.0)}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "pi          50.00 ns     0.5%     20.00M\n");
}

TEST_CASE("items and bytes together") {
    bench::Result both = measured("copy", 1000.0);
    both.items = 100;
    both.bytes = 800;
    CHECK(report({both}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "copy        1.000 us     0.5%     1.000M     100.0M items/s, 800.0 MB/s\n");
}

TEST_CASE("a failure takes the place of the numbers") {
    CHECK(report({measured("sort", 1000.0), failed("parse", "unexpected end of input")}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "sort        1.000 us     0.5%     1.000M\n"
          "parse       failed: unexpected end of input\n");
}

TEST_CASE("a spread above 5% is marked, with a warning below the table") {
    CHECK(report({measured("steady", 1000.0, 5.0), measured("busy", 3.1e6, 6.1)}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "steady      1.000 us     5.0%     1.000M\n"
          "busy        3.100 ms     6.1% !   322.6\n"
          "\n"
          "! the spread is above 5%: the machine may have been busy\n");
}

TEST_CASE("a run under 10 ticks of the clock is marked, with a warning") {
    bench::Result fast = measured("add", 2.5);
    fast.measurement->resolution = 1ns;
    CHECK(report({fast, measured("sort", 10.0)}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "add         2.500 ns *   0.5%     400.0M\n"
          "sort        10.00 ns     0.5%     100.0M\n"
          "\n"
          "* too fast to measure reliably: under 10.00 ns a run, 10 times the clock's resolution\n");
}

TEST_CASE("code built without optimization or without NDEBUG gets a warning") {
    for (const bench::Build build : {bench::Build{false, true}, bench::Build{true, false}}) {
        bench::Result result = measured("sort", 1000.0);
        result.build = build;
        CHECK(report({result}) ==
              "benchmark   time/run     spread   runs/s     throughput\n"
              "sort        1.000 us     0.5%     1.000M\n"
              "\n"
              "these results are from code built without optimization; build with --release\n");
    }
}

TEST_CASE("the warnings come in order: the build, the spread, then speed") {
    bench::Result result = measured("add", 2.5, 9.0);
    result.build = bench::Build{false, false};
    CHECK(report({result}) ==
          "benchmark   time/run     spread   runs/s     throughput\n"
          "add         2.500 ns *   9.0% !   400.0M\n"
          "\n"
          "these results are from code built without optimization; build with --release\n"
          "! the spread is above 5%: the machine may have been busy\n"
          "* too fast to measure reliably: under 10.00 ns a run, 10 times the clock's resolution\n");
}

TEST_CASE("nothing to run") { CHECK(report({}) == "no benchmarks to run\n"); }

TEST_CASE("check mode reports failures and a count") {
    CHECK(report({bench::Result{}}, bench::Mode::check) == "checked 1 benchmark\n");
    bench::Result ok;
    ok.name = "sort";
    CHECK(report({ok, failed("parse", "unexpected end"), failed("hash", "oops")}, bench::Mode::check) ==
          "parse: failed: unexpected end\n"
          "hash: failed: oops\n"
          "checked 3 benchmarks: 2 failed\n");
}

TEST_CASE("a run on a fake clock, from the start") {
    support::FakeClock clock;
    bench::Registry registry;
    registry.add(support::benchmark("vector push_back 1000", [&] { clock.advance(1230ns); }));
    registry.add(support::benchmark("sort 10000", [&] {
        bench::items(10'000);
        bench::measure([&] { clock.advance(412us); });
    }));
    registry.add(support::benchmark("parse 1 MB", [&] {
        bench::bytes(1'000'000);
        bench::measure([&] { clock.advance(3100us); });
    }));
    registry.add(support::benchmark("broken", [] { throw std::runtime_error("no input"); }));
    std::ostringstream out;
    bench::ConsoleReporter reporter(out);
    bench::run(support::all(registry), bench::Settings{}, clock, reporter);
    CHECK(out.str() ==
          "benchmark               time/run     spread   runs/s     throughput\n"
          "vector push_back 1000   1.230 us     0.0%     813.0k\n"
          "sort 10000              412.0 us     0.0%     2.427k     24.27M items/s\n"
          "parse 1 MB              3.100 ms     0.0%     322.6      322.6 MB/s\n"
          "broken                  failed: no input\n");
}
