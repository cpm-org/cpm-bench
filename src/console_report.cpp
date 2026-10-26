#include "units.hpp"

#include <cpm/bench/console_report.hpp>

#include <algorithm>
#include <format>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpm::bench {
namespace {

// The columns after the name, each with the space that follows it. The time and the spread leave room for a mark.
constexpr std::size_t time_width = 13;
constexpr std::size_t spread_width = 9;
constexpr std::size_t runs_width = 11;
constexpr std::size_t name_gap = 3;

constexpr std::string_view noisy_mark = "!";
constexpr std::string_view too_fast_mark = "*";

std::string padded(std::string text, std::size_t width) {
    if (text.size() < width) text.append(width - text.size(), ' ');
    return text;
}

std::string without_trailing_spaces(std::string line) {
    line.erase(line.find_last_not_of(' ') + 1);
    return line;
}

std::string throughput(const Result& result) {
    std::vector<std::string> rates;
    if (const auto items = items_per_second(result)) rates.push_back(detail::format_count(*items) + " items/s");
    if (const auto bytes = bytes_per_second(result)) rates.push_back(detail::format_byte_rate(*bytes));
    std::string text;
    for (const std::string& rate : rates) text += (text.empty() ? "" : ", ") + rate;
    return text;
}

// `text`, followed by `mark` when `marked`.
std::string with_mark(std::string text, bool marked, std::string_view mark) {
    if (marked) text += ' ' + std::string(mark);
    return text;
}

std::string measured_cells(const Result& result, const Measurement& measurement) {
    const std::optional<double> runs = runs_per_second(measurement);
    return padded(with_mark(detail::format_time(measurement.time.median), too_fast(measurement), too_fast_mark),
                  time_width) +
           padded(with_mark(detail::format_percent(measurement.time.spread), noisy(measurement), noisy_mark),
                  spread_width) +
           padded(runs ? detail::format_count(*runs) : "-", runs_width) + throughput(result);
}

std::string row(const Result& result, std::size_t name_width) {
    std::string line = padded(result.name, name_width);
    if (result.error) {
        line += "failed: " + *result.error;
    } else if (result.measurement) {
        line += measured_cells(result, *result.measurement);
    }
    return without_trailing_spaces(std::move(line));
}

std::vector<std::string> warnings(std::span<const Result> results) {
    const auto unoptimized = [](const Result& result) { return !result.build.release(); };
    const auto busy = [](const Result& result) { return result.measurement && noisy(*result.measurement); };
    const auto fast = [](const Result& result) { return result.measurement && too_fast(*result.measurement); };
    std::vector<std::string> lines;
    if (std::ranges::any_of(results, unoptimized)) {
        lines.emplace_back("these results are from code built without optimization; build with --release");
    }
    if (std::ranges::any_of(results, busy)) {
        lines.push_back(std::format("{} the spread is above 5%: the machine may have been busy", noisy_mark));
    }
    if (const auto first = std::ranges::find_if(results, fast); first != results.end()) {
        const double threshold = 10 * static_cast<double>(first->measurement->resolution.count());
        lines.push_back(std::format("{} too fast to measure reliably: under {} a run, 10 times the clock's resolution",
                                    too_fast_mark, detail::format_time(threshold)));
    }
    return lines;
}

std::string plural(std::size_t count, std::string_view noun) {
    return std::format("{} {}{}", count, noun, count == 1 ? "" : "s");
}

}  // namespace

void ConsoleReporter::run_started(std::span<const Benchmark* const> benchmarks, const Settings& settings) {
    mode_ = settings.mode;
    if (mode_ == Mode::check) return;
    if (benchmarks.empty()) {
        *out_ << "no benchmarks to run\n";
        return;
    }
    std::size_t longest = std::string_view("benchmark").size();
    for (const Benchmark* benchmark : benchmarks) longest = std::max(longest, benchmark->name.size());
    name_width_ = longest + name_gap;
    *out_ << without_trailing_spaces(padded("benchmark", name_width_) + padded("time/run", time_width) +
                                     padded("spread", spread_width) + padded("runs/s", runs_width) + "throughput")
          << '\n';
}

void ConsoleReporter::benchmark_finished(const Result& result) {
    if (mode_ == Mode::check) {
        if (result.failed()) *out_ << result.name << ": failed: " << *result.error << '\n';
        return;
    }
    *out_ << row(result, name_width_) << std::endl;  // a row at a time, so a slow benchmark shows the ones before it
}

void ConsoleReporter::run_finished(std::span<const Result> results) {
    if (mode_ == Mode::check) {
        const auto failed = static_cast<std::size_t>(std::ranges::count_if(results, &Result::failed));
        *out_ << "checked " << plural(results.size(), "benchmark");
        if (failed > 0) *out_ << ": " << failed << " failed";
        *out_ << '\n';
        return;
    }
    const std::vector<std::string> lines = warnings(results);
    if (lines.empty()) return;
    *out_ << '\n';
    for (const std::string& line : lines) *out_ << line << '\n';
}

}  // namespace cpm::bench
