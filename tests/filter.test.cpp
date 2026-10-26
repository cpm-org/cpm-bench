// Choosing benchmarks by name (docs/spec.md §5).
#include <filter.hpp>

#include <cpm/test.hpp>

#include <string>
#include <vector>

#include "support/fakes.hpp"

using cpm::bench::detail::Filter;
using cpm::bench::detail::matches;

namespace {

cpm::bench::Registry registry_of(const std::vector<std::string>& names) {
    cpm::bench::Registry registry;
    for (const std::string& name : names) registry.add(support::benchmark(name, [] {}));
    return registry;
}

std::vector<std::string> names(const std::vector<const cpm::bench::Benchmark*>& benchmarks) {
    std::vector<std::string> result;
    for (const auto* benchmark : benchmarks) result.push_back(benchmark->name);
    return result;
}

}  // namespace

TEST_CASE("a pattern without * matches only itself") {
    CHECK(matches("sort", "sort"));
    CHECK_FALSE(matches("sort", "sort 100"));
    CHECK_FALSE(matches("sort", "Sort"));
    CHECK_FALSE(matches("sort", ""));
}

TEST_CASE("* matches any text, including none") {
    CHECK(matches("sort*", "sort"));
    CHECK(matches("sort*", "sort 10000"));
    CHECK(matches("*1000", "push_back 1000"));
    CHECK(matches("*back*", "push_back 1000"));
    CHECK(matches("s*t*0", "sort 10000"));
    CHECK(matches("*", ""));
    CHECK(matches("**", "anything"));
    CHECK_FALSE(matches("s*x", "sort"));
    CHECK_FALSE(matches("*a*b", "ba"));
}

TEST_CASE("no patterns choose every benchmark, in order") {
    const auto registry = registry_of({"sort 100", "sort 10000", "parse"});
    CHECK(names(Filter({}, {}).select(registry)) == std::vector<std::string>{"sort 100", "sort 10000", "parse"});
}

TEST_CASE("patterns choose what any of them matches") {
    const auto registry = registry_of({"sort 100", "sort 10000", "parse", "hash"});
    CHECK(names(Filter({"sort*", "hash"}, {}).select(registry)) ==
          std::vector<std::string>{"sort 100", "sort 10000", "hash"});
}

TEST_CASE("exclusions leave out what they match") {
    const auto registry = registry_of({"sort 100", "sort 10000", "parse"});
    CHECK(names(Filter({}, {"*10000"}).select(registry)) == std::vector<std::string>{"sort 100", "parse"});
    CHECK(names(Filter({"sort*"}, {"*100"}).select(registry)) == std::vector<std::string>{"sort 10000"});
    CHECK(Filter({}, {"*"}).select(registry).empty());
}

TEST_CASE("patterns that match no benchmark are reported as written") {
    const auto registry = registry_of({"sort 100", "parse"});
    CHECK(Filter({"sort*"}, {"parse"}).unmatched(registry).empty());
    CHECK(Filter({"prase", "sort*", "hash*"}, {"json", "parse"}).unmatched(registry) ==
          std::vector<std::string>{"prase", "hash*", "~json"});
}

TEST_CASE("a pattern that matches only excluded benchmarks still matches") {
    const auto registry = registry_of({"sort 100", "parse"});
    const Filter filter({"sort*"}, {"sort*"});
    CHECK(filter.unmatched(registry).empty());
    CHECK(filter.select(registry).empty());
}
