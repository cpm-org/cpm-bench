#include "command_line.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace cpm::args {

template <>
struct Reader<bench::detail::SampleCount> {
    static bench::detail::SampleCount read(std::string_view text) {
        const auto count = Reader<std::size_t>::read(text);
        if (count == 0) throw ValueError("is out of range: expected 1 or more");
        return {count};
    }
};

template <>
struct Reader<bench::detail::Milliseconds> {
    static bench::detail::Milliseconds read(std::string_view text) {
        const double milliseconds = Reader<double>::read(text);
        if (!(milliseconds > 0)) throw ValueError("is out of range: expected a number above 0");
        return {milliseconds};
    }
};

}  // namespace cpm::args

namespace cpm::bench::detail {

CommandLine::CommandLine(std::string program)
    : parser_(std::move(program), "Runs the benchmarks whose names match a pattern, or all of them.") {
    parser_.flag(list_, "--list", "-l").help("list the benchmarks instead of running them");
    parser_.option(samples_, "--samples").value_name("N").default_text("11").help("samples per benchmark");
    parser_.option(sample_time_, "--sample-time")
        .value_name("MS")
        .default_text("10")
        .help("the least time per sample, in ms");
    parser_.option(json_, "--json").value_name("FILE").help("also write the results to FILE as JSON");
    parser_.flag(check_, "--check").env("CPM_TEST").help("run each benchmark once, unmeasured");
    parser_.flag(measure_, "--measure").help("measure, even with --check or under cpm test");
    parser_.positional(patterns_, "PATTERN")
        .optional()
        .help("* matches any text; a leading ~ leaves out what it matches");
}

args::Outcome CommandLine::parse(std::span<const std::string_view> arguments, const Environment& environment) {
    return parser_.parse(arguments, environment);
}

Options CommandLine::options() const {
    Options options;
    options.list = list_;
    options.settings.mode = check_ && !measure_ ? Mode::check : Mode::measure;
    options.settings.samples = samples_.value;
    options.settings.sample_time =
        std::chrono::nanoseconds(std::llround(std::min(sample_time_.value, 3.6e6) * 1e6));  // up to an hour
    for (const std::string& pattern : patterns_) {
        if (pattern.starts_with('~')) {
            options.exclude.push_back(pattern.substr(1));
        } else {
            options.include.push_back(pattern);
        }
    }
    options.json = json_;
    return options;
}

}  // namespace cpm::bench::detail
