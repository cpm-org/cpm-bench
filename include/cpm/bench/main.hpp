// The runner (docs/spec.md §5): the command line, choosing benchmarks, running them and the reports.
#pragma once

#include <cpm/bench/clock.hpp>
#include <cpm/bench/registry.hpp>

#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cpm::bench {

// The value of an environment variable, or nullopt when it isn't set.
using Environment = std::function<std::optional<std::string>(std::string_view name)>;

// The process's environment.
Environment system_environment();

// The steady clock main() measures with.
Clock& steady_clock();

// What main() works with besides its arguments: the process's own, unless a test supplies its own.
struct System {
    std::string program = "bench";  // the name help and errors show
    Environment environment = system_environment();
    Clock* clock = &steady_clock();
    std::ostream* out = &std::cout;  // the reports, help and --list
    std::ostream* err = &std::cerr;  // bad command lines, and a JSON file that can't be written
};

// Acts on `arguments` (without the program's name) for `registry`. Returns the exit status: 0 when everything ran, 1
// when a benchmark failed or the JSON file couldn't be written, 2 for a bad command line.
int main(const Registry& registry, std::span<const std::string_view> arguments, const System& system = {});

// What CPM_BENCH_MAIN() calls: registry(), the arguments, and the program's name from argv[0]. A program with its own
// main() can call it after its own setup.
int main(int argc, const char* const* argv);

}  // namespace cpm::bench
