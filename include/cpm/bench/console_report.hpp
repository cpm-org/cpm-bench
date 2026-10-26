// The report on the console (docs/spec.md §6.1).
#pragma once

#include <cpm/bench/run.hpp>

#include <cstddef>
#include <iosfwd>
#include <span>

namespace cpm::bench {

// When measuring: a table with a row for each benchmark as it finishes, and warnings below it. In check mode: a line
// for each failure, and a count.
class ConsoleReporter final : public Reporter {
public:
    explicit ConsoleReporter(std::ostream& out) noexcept : out_(&out) {}

    void run_started(std::span<const Benchmark* const> benchmarks, const Settings& settings) override;
    void benchmark_finished(const Result& result) override;
    void run_finished(std::span<const Result> results) override;

private:
    std::ostream* out_;
    Mode mode_ = Mode::measure;
    std::size_t name_width_ = 0;
};

}  // namespace cpm::bench
