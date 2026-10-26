#include "command_line.hpp"
#include "filter.hpp"

#include <cpm/bench/console_report.hpp>
#include <cpm/bench/json_report.hpp>
#include <cpm/bench/main.hpp>
#include <cpm/bench/run.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <ostream>
#include <vector>

namespace cpm::bench {
namespace {

// The benchmarks `options` choose. A pattern that matches nothing is a bad command line, so a typo doesn't pass
// silently.
std::vector<const Benchmark*> choose(const Registry& registry, const detail::Options& options,
                                     const args::Command& command) {
    const detail::Filter filter(options.include, options.exclude);
    const std::vector<std::string> unmatched = filter.unmatched(registry);
    if (!unmatched.empty()) {
        std::string patterns;
        for (const std::string& pattern : unmatched) {
            patterns += std::format("{}'{}'", patterns.empty() ? "" : ", ", pattern);
        }
        throw args::UsageError(std::format("no benchmark matches {}; --list shows their names", patterns), command);
    }
    return filter.select(registry);
}

bool write_file(const std::filesystem::path& path, const std::string& text, std::ostream& err) {
    std::ofstream file(path, std::ios::binary);
    file << text;
    file.close();
    if (file) return true;
    err << "error: couldn't write the results to " << path.string() << '\n';
    return false;
}

// The program's name from argv[0]: without its directory, and without .exe.
std::string program_name(const char* argv0) {
    std::string_view name = argv0 != nullptr ? argv0 : "";
    if (const auto slash = name.find_last_of("/\\"); slash != std::string_view::npos) name.remove_prefix(slash + 1);
    if (name.ends_with(".exe")) name.remove_suffix(4);
    return name.empty() ? "bench" : std::string(name);
}

}  // namespace

Environment system_environment() { return args::system_environment(); }

Clock& steady_clock() {
    static SteadyClock clock;
    return clock;
}

int main(const Registry& registry, std::span<const std::string_view> arguments, const System& system) {
    detail::CommandLine command_line(system.program);
    try {
        if (command_line.parse(arguments, system.environment).action != args::Outcome::Action::run) {
            *system.out << command_line.parser().help_text();
            return 0;
        }
        const detail::Options options = command_line.options();
        const std::vector<const Benchmark*> chosen = choose(registry, options, command_line.parser());
        if (options.list) {
            for (const Benchmark* benchmark : chosen) *system.out << benchmark->name << '\n';
            return 0;
        }
        ConsoleReporter reporter(*system.out);
        const std::vector<Result> results = run(chosen, options.settings, *system.clock, reporter);
        const bool all_ran = std::ranges::none_of(results, &Result::failed);
        const bool written = !options.json || write_file(*options.json, json_report(results), *system.err);
        return all_ran && written ? 0 : 1;
    } catch (const args::UsageError& error) {
        *system.err << "error: " << error.what() << "\n\n"
                    << command_line.parser().usage_text(error.command()) << "\n\n"
                    << "For more information, try '" << error.command().path() << " --help'.\n";
        return 2;
    }
}

int main(int argc, const char* const* argv) {
    std::vector<std::string_view> arguments;
    for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
    System system;
    system.program = program_name(argc > 0 ? argv[0] : nullptr);
    return main(registry(), arguments, system);
}

}  // namespace cpm::bench
