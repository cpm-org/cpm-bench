#include "calibration.hpp"
#include "context.hpp"
#include "sampler.hpp"
#include "statistics.hpp"

#include <cpm/bench/run.hpp>

#include <exception>
#include <stdexcept>

namespace cpm::bench {
namespace {

Measurement take_measurement(const detail::Batch& batch, Clock& clock, const Settings& settings) {
    const std::uint64_t runs = detail::calibrate(batch, clock, settings.sample_time);
    const std::vector<double> times = detail::sample(batch, clock, runs, settings.samples);
    return Measurement{detail::summarize(times), settings.samples, runs, clock.resolution()};
}

// What a benchmark's body runs in: it keeps what items() and bytes() say, and measures what measure() is given (in
// check mode, runs it once). A body that doesn't call measure() is measured itself.
class BodyContext final : public detail::Context {
public:
    BodyContext(const Settings& settings, Clock& clock) noexcept : settings_(settings), clock_(clock) {}

    void run(const std::function<void()>& body) {
        body();
        if (state_ == State::measured || settings_.mode == Mode::check) return;
        state_ = State::whole_body;
        measurement_ = take_measurement(
            [&body](std::uint64_t runs) {
                for (std::uint64_t run = 0; run < runs; ++run) body();
            },
            clock_, settings_);
    }

    void measure(const detail::Batch& batch) override {
        if (state_ == State::measured) throw std::logic_error("measure() was called twice in one benchmark");
        if (state_ == State::whole_body)
            throw std::logic_error("measure() was called on a later run of the body, but not on the first");
        state_ = State::measured;
        if (settings_.mode == Mode::check) {
            batch(1);
        } else {
            measurement_ = take_measurement(batch, clock_, settings_);
        }
    }

    void items(std::uint64_t count) override { items_ = count; }
    void bytes(std::uint64_t count) override { bytes_ = count; }

    const std::optional<Measurement>& measurement() const noexcept { return measurement_; }
    std::optional<std::uint64_t> item_count() const noexcept { return items_; }
    std::optional<std::uint64_t> byte_count() const noexcept { return bytes_; }

private:
    enum class State {
        first_run,   // the body hasn't returned yet, nor called measure()
        measured,    // the body called measure()
        whole_body,  // the body is the code measured
    };

    const Settings& settings_;
    Clock& clock_;
    State state_ = State::first_run;
    std::optional<Measurement> measurement_;
    std::optional<std::uint64_t> items_;
    std::optional<std::uint64_t> bytes_;
};

Result run_one(const Benchmark& benchmark, const Settings& settings, Clock& clock) {
    Result result;
    result.name = benchmark.name;
    result.build = benchmark.build;
    BodyContext context(settings, clock);
    try {
        const detail::ContextScope scope(context);
        context.run(benchmark.body);
        result.measurement = context.measurement();
    } catch (const std::exception& error) {
        result.error = error.what();
    } catch (...) {
        result.error = "an exception that isn't a std::exception";
    }
    result.items = context.item_count();
    result.bytes = context.byte_count();
    return result;
}

}  // namespace

std::vector<Result> run(std::span<const Benchmark* const> benchmarks, const Settings& settings, Clock& clock,
                        Reporter& reporter) {
    reporter.run_started(benchmarks, settings);
    std::vector<Result> results;
    results.reserve(benchmarks.size());
    for (const Benchmark* benchmark : benchmarks) {
        results.push_back(run_one(*benchmark, settings, clock));
        reporter.benchmark_finished(results.back());
    }
    reporter.run_finished(results);
    return results;
}

}  // namespace cpm::bench
