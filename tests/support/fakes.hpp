// Shared by the tests: a clock that moves only when told, benchmarks built in code, and a reporter that records.
#pragma once

#include <cpm/bench.hpp>

#include <chrono>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace support {

namespace bench = cpm::bench;

// now() stays put until advance() moves it, so a benchmark's body says exactly how long it takes.
class FakeClock final : public bench::Clock {
public:
    explicit FakeClock(std::chrono::nanoseconds resolution = std::chrono::nanoseconds(1)) : resolution_(resolution) {}

    std::chrono::nanoseconds now() override { return now_; }
    std::chrono::nanoseconds resolution() const override { return resolution_; }

    void advance(std::chrono::nanoseconds by) { now_ += by; }

private:
    std::chrono::nanoseconds now_{1'000'000};
    std::chrono::nanoseconds resolution_;
};

inline bench::Benchmark benchmark(std::string name, std::function<void()> body,
                                  bench::Build build = bench::Build{true, true}) {
    bench::Benchmark result;
    result.name = std::move(name);
    result.body = std::move(body);
    result.file = "bench.cpp";
    result.line = 1;
    result.build = build;
    return result;
}

// The benchmarks of `registry`, as run() takes them.
inline std::vector<const bench::Benchmark*> all(const bench::Registry& registry) {
    std::vector<const bench::Benchmark*> benchmarks;
    for (const bench::Benchmark& benchmark : registry.benchmarks()) benchmarks.push_back(&benchmark);
    return benchmarks;
}

// Records what it is told, as text: "started 2", "finished sort", "ended 2".
class RecordingReporter final : public bench::Reporter {
public:
    std::vector<std::string> events;

    void run_started(std::span<const bench::Benchmark* const> benchmarks, const bench::Settings&) override {
        events.push_back("started " + std::to_string(benchmarks.size()));
    }
    void benchmark_finished(const bench::Result& result) override { events.push_back("finished " + result.name); }
    void run_finished(std::span<const bench::Result> results) override {
        events.push_back("ended " + std::to_string(results.size()));
    }
};

// Runs a single benchmark with `settings` on `clock`.
inline bench::Result run_one(std::function<void()> body, bench::Clock& clock, const bench::Settings& settings = {}) {
    bench::Registry registry;
    registry.add(benchmark("benchmark", std::move(body)));
    RecordingReporter reporter;
    return bench::run(all(registry), settings, clock, reporter).at(0);
}

}  // namespace support
