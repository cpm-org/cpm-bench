// The runner's command line (docs/spec.md §5), read with cpm-args.
#pragma once

#include <cpm/args.hpp>
#include <cpm/bench/main.hpp>
#include <cpm/bench/run.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::bench::detail {

struct Options {
    bool list = false;
    Settings settings;
    std::vector<std::string> include;  // the patterns of the benchmarks to run; none: all of them
    std::vector<std::string> exclude;  // the patterns given with a ~, without it
    std::optional<std::filesystem::path> json;
};

// A whole number of samples, at least 1.
struct SampleCount {
    std::size_t value = 11;
};

// A time in milliseconds, above 0.
struct Milliseconds {
    double value = 10;
};

class CommandLine {
public:
    explicit CommandLine(std::string program);

    CommandLine(const CommandLine&) = delete;  // the parser holds references to the members
    CommandLine& operator=(const CommandLine&) = delete;

    // Reads `arguments` (without the program's name). Throws args::UsageError. The outcome says whether help was asked
    // for; when it wasn't, options() has what the command line asks for.
    args::Outcome parse(std::span<const std::string_view> arguments, const Environment& environment);

    Options options() const;

    const args::Parser& parser() const noexcept { return parser_; }

private:
    bool list_ = false;
    bool check_ = false;
    bool measure_ = false;
    SampleCount samples_;
    Milliseconds sample_time_;
    std::optional<std::filesystem::path> json_;
    std::vector<std::string> patterns_;
    args::Parser parser_;
};

}  // namespace cpm::bench::detail
