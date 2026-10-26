// The runner (docs/spec.md §5): the command line, check mode, the exit status and the JSON file.
#include <cpm/bench.hpp>
#include <cpm/json.hpp>
#include <cpm/test.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "support/fakes.hpp"

using namespace std::chrono_literals;
namespace bench = cpm::bench;
namespace fs = std::filesystem;

namespace {

// Three benchmarks on a fake clock, and main() run on them with chosen arguments and environment variables.
struct Program {
    support::FakeClock clock;
    bench::Registry registry;
    std::map<std::string, std::string, std::less<>> environment;
    std::ostringstream out;
    std::ostringstream err;
    int sort_runs = 0;

    Program() {
        registry.add(support::benchmark("sort 100", [&] {
            ++sort_runs;
            clock.advance(1us);
        }));
        registry.add(support::benchmark("sort 10000", [&] { clock.advance(200us); }));
        registry.add(support::benchmark("parse", [&] { clock.advance(3ms); }));
    }

    int run(std::initializer_list<std::string_view> arguments) {
        bench::System system;
        system.program = "strings.test";
        system.environment = [this](std::string_view name) -> std::optional<std::string> {
            if (const auto it = environment.find(name); it != environment.end()) return it->second;
            return std::nullopt;
        };
        system.clock = &clock;
        system.out = &out;
        system.err = &err;
        const std::vector<std::string_view> list(arguments);
        return bench::main(registry, list, system);
    }
};

// A path in the temporary directory, removed when the test ends.
struct TemporaryFile {
    fs::path path = fs::temp_directory_path() / "cpm-bench-main-test.json";
    TemporaryFile() { fs::remove(path); }
    ~TemporaryFile() { fs::remove(path); }
};

std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST_CASE("without arguments every benchmark is measured") {
    Program program;
    CHECK(program.run({}) == 0);
    CHECK(program.out.str() ==
          "benchmark    time/run     spread   runs/s     throughput\n"
          "sort 100     1.000 us     0.0%     1.000M\n"
          "sort 10000   200.0 us     0.0%     5.000k\n"
          "parse        3.000 ms     0.0%     333.3\n");
    CHECK(program.err.str().empty());
}

TEST_CASE("patterns choose benchmarks, and ~ leaves some out") {
    Program program;
    CHECK(program.run({"--check", "sort*", "~*100"}) == 0);
    CHECK(program.out.str() == "checked 1 benchmark\n");
    CHECK(program.sort_runs == 0);
}

TEST_CASE("--list lists the chosen benchmarks instead of running them") {
    Program program;
    CHECK(program.run({"--list"}) == 0);
    CHECK(program.out.str() == "sort 100\nsort 10000\nparse\n");
    Program chosen;
    CHECK(chosen.run({"-l", "*0"}) == 0);
    CHECK(chosen.out.str() == "sort 100\nsort 10000\n");
    CHECK(chosen.sort_runs == 0);
}

TEST_CASE("--samples and --sample-time set the measurement") {
    Program program;
    TemporaryFile json;
    CHECK(program.run({"--samples", "3", "--sample-time", "0.5", "--json", json.path.string(), "sort 100"}) == 0);
    const cpm::json::Value entry = cpm::json::parse(read(json.path)).at("benchmarks").as_array().at(0);
    CHECK(entry.at("samples").get<int>() == 3);
    CHECK(entry.at("runs_per_sample").get<int>() == 512);  // 512 runs of 1us take 0.5 ms
}

TEST_CASE("under cpm test, CPM_TEST turns on check mode") {
    Program program;
    program.environment["CPM_TEST"] = "1";
    CHECK(program.run({}) == 0);
    CHECK(program.out.str() == "checked 3 benchmarks\n");
    CHECK(program.sort_runs == 1);
}

TEST_CASE("--check turns on check mode") {
    Program program;
    CHECK(program.run({"--check"}) == 0);
    CHECK(program.out.str() == "checked 3 benchmarks\n");
}

TEST_CASE("--measure measures even under cpm test") {
    Program program;
    program.environment["CPM_TEST"] = "1";
    CHECK(program.run({"--measure", "parse"}) == 0);
    CHECK(program.out.str().starts_with("benchmark   time/run"));
    Program both;
    CHECK(both.run({"--check", "--measure", "parse"}) == 0);
    CHECK(both.out.str().starts_with("benchmark   time/run"));
}

TEST_CASE("a failed benchmark makes the exit status 1") {
    Program program;
    program.registry.add(support::benchmark("broken", [] { throw std::runtime_error("no input"); }));
    CHECK(program.run({"--check"}) == 1);
    CHECK(program.out.str() == "broken: failed: no input\nchecked 4 benchmarks: 1 failed\n");
    Program measuring;
    measuring.registry.add(support::benchmark("broken", [] { throw std::runtime_error("no input"); }));
    CHECK(measuring.run({"parse", "broken"}) == 1);
}

TEST_CASE("--json writes the results to a file") {
    Program program;
    TemporaryFile json;
    CHECK(program.run({"--json", json.path.string()}) == 0);
    const cpm::json::Value document = cpm::json::parse(read(json.path));
    CHECK(document.at("benchmarks").as_array().size() == 3);
    CHECK(document.at("benchmarks").as_array().at(2).at("median_ns").get<double>() == 3e6);
    CHECK(document.at("optimized").as_boolean());
}

TEST_CASE("a JSON file that can't be written is an error, with exit status 1") {
    Program program;
    const fs::path path = fs::temp_directory_path() / "cpm-bench-no-such-directory" / "results.json";
    CHECK(program.run({"--json", path.string(), "parse"}) == 1);
    CHECK(program.err.str() == "error: couldn't write the results to " + path.string() + "\n");
    CHECK(program.out.str().starts_with("benchmark"));
}

TEST_CASE("a pattern that matches nothing is a bad command line") {
    Program program;
    CHECK(program.run({"sort*", "prase", "~json"}) == 2);
    CHECK(program.out.str().empty());
    CHECK(program.err.str() ==
          "error: no benchmark matches 'prase', '~json'; --list shows their names\n"
          "\n"
          "Usage: strings.test [OPTIONS] [PATTERN...]\n"
          "\n"
          "For more information, try 'strings.test --help'.\n");
}

TEST_CASE("an unknown option is a bad command line") {
    Program program;
    CHECK(program.run({"--sample", "3"}) == 2);
    CHECK(program.err.str().starts_with("error: unknown option --sample (did you mean --samples?)\n"));
}

TEST_CASE("--samples and --sample-time must be above 0") {
    Program samples;
    CHECK(samples.run({"--samples", "0"}) == 2);
    CHECK(samples.err.str().starts_with("error: "));
    CHECK(samples.err.str().find("is out of range: expected 1 or more") != std::string::npos);
    Program time;
    CHECK(time.run({"--sample-time", "-1"}) == 2);
    CHECK(time.err.str().find("is out of range: expected a number above 0") != std::string::npos);
    Program text;
    CHECK(text.run({"--samples", "many"}) == 2);
}

TEST_CASE("--help shows the options, and runs nothing") {
    Program program;
    CHECK(program.run({"--help"}) == 0);
    CHECK(program.out.str() ==
          "Runs the benchmarks whose names match a pattern, or all of them.\n"
          "\n"
          "Usage: strings.test [OPTIONS] [PATTERN...]\n"
          "\n"
          "Arguments:\n"
          "  [PATTERN...]  * matches any text; a leading ~ leaves out what it matches\n"
          "\n"
          "Options:\n"
          "  -l, --list              list the benchmarks instead of running them\n"
          "      --samples <N>       samples per benchmark (default: 11)\n"
          "      --sample-time <MS>  the least time per sample, in ms (default: 10)\n"
          "      --json <FILE>       also write the results to FILE as JSON\n"
          "      --check             run each benchmark once, unmeasured [env: CPM_TEST]\n"
          "      --measure           measure, even with --check or under cpm test\n"
          "  -h, --help              show this help\n");
    CHECK(program.sort_runs == 0);
}

TEST_CASE("no benchmarks at all is not an error") {
    bench::Registry empty;
    std::ostringstream out;
    bench::System system;
    system.out = &out;
    system.environment = [](std::string_view) { return std::optional<std::string>(); };
    CHECK(bench::main(empty, {}, system) == 0);
    CHECK(out.str() == "no benchmarks to run\n");
}
