#include <cpm/bench/registry.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>

namespace cpm::bench {

void Registry::add(Benchmark benchmark) {
    const auto same_name = [&](const Benchmark& other) { return other.name == benchmark.name; };
    if (const auto it = std::ranges::find_if(benchmarks_, same_name); it != benchmarks_.end()) {
        throw std::logic_error(std::format("two benchmarks are named \"{}\": at {}:{} and at {}:{}", benchmark.name,
                                           it->file, it->line, benchmark.file, benchmark.line));
    }
    benchmarks_.push_back(std::move(benchmark));
}

Registry& registry() {
    static Registry instance;
    return instance;
}

bool add(std::string name, std::function<void()> body, Build build, std::source_location where) {
    const auto line = static_cast<int>(where.line());
    registry().add(Benchmark{std::move(name), std::move(body), where.file_name(), line, build});
    return true;
}

namespace detail {

bool add_benchmark(std::string name, void (*body)(), const char* file, int line, Build build) {
    registry().add(Benchmark{std::move(name), body, file, line, build});
    return true;
}

}  // namespace detail
}  // namespace cpm::bench
