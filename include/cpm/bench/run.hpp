// Running benchmarks (docs/spec.md §4, §5): measuring them, or checking that they run, and telling a Reporter.
#pragma once

#include <cpm/bench/clock.hpp>
#include <cpm/bench/registry.hpp>
#include <cpm/bench/result.hpp>

#include <chrono>
#include <cstddef>
#include <span>
#include <vector>

namespace cpm::bench {

enum class Mode {
    measure,  // calibrate, warm up and take samples
    check,    // run each benchmark's code once, unmeasured
};

struct Settings {
    Mode mode = Mode::measure;
    std::size_t samples = 11;  // at least 1
    std::chrono::nanoseconds sample_time = std::chrono::milliseconds(10);  // the least time per sample
};

// Told about a run as it goes, to report it.
class Reporter {
public:
    virtual ~Reporter() = default;

    virtual void run_started(std::span<const Benchmark* const> benchmarks, const Settings& settings) = 0;
    virtual void benchmark_finished(const Result& result) = 0;
    virtual void run_finished(std::span<const Result> results) = 0;

protected:
    Reporter() = default;
    Reporter(const Reporter&) = default;
    Reporter& operator=(const Reporter&) = default;
};

// Runs `benchmarks` in order. One that throws is a failed Result, and the others still run.
std::vector<Result> run(std::span<const Benchmark* const> benchmarks, const Settings& settings, Clock& clock,
                        Reporter& reporter);

}  // namespace cpm::bench
