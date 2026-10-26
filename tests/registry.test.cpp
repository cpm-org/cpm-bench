// Defining and registering benchmarks (docs/spec.md §3, §7).
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

#include "support/fakes.hpp"

namespace bench = cpm::bench;

namespace {

int macro_runs = 0;

const bench::Benchmark* find(std::string_view name) {
    const auto& benchmarks = bench::registry().benchmarks();
    const auto it = std::ranges::find(benchmarks, name, &bench::Benchmark::name);
    return it == benchmarks.end() ? nullptr : &*it;
}

}  // namespace

constexpr int macro_line = __LINE__ + 1;
BENCHMARK("registered by the macro") { ++macro_runs; }

CPM_BENCHMARK("registered by the long macro") {}

constexpr int add_line = __LINE__ + 1;
[[maybe_unused]] const bool registered_by_add = bench::add("registered by add()", [] {});

TEST_CASE("BENCHMARK registers in registry() with its place") {
    const bench::Benchmark* benchmark = find("registered by the macro");
    REQUIRE(benchmark != nullptr);
    CHECK(benchmark->file.ends_with("registry.test.cpp"));
    CHECK(benchmark->line == macro_line);
    benchmark->body();
    CHECK(macro_runs == 1);
    CHECK(find("registered by the long macro") != nullptr);
}

TEST_CASE("BENCHMARK records how the code around it is compiled") {
    const bench::Benchmark* benchmark = find("registered by the macro");
    REQUIRE(benchmark != nullptr);
#if defined(__OPTIMIZE__)
    CHECK(benchmark->build.optimized);
#elif defined(__GNUC__) || defined(__clang__)
    CHECK_FALSE(benchmark->build.optimized);
#endif
#if defined(NDEBUG)
    CHECK(benchmark->build.ndebug);
#else
    CHECK_FALSE(benchmark->build.ndebug);
    CHECK_FALSE(benchmark->build.release());
#endif
}

TEST_CASE("add() registers with the caller's place and build") {
    CHECK(registered_by_add);
    const bench::Benchmark* benchmark = find("registered by add()");
    REQUIRE(benchmark != nullptr);
    CHECK(benchmark->file.ends_with("registry.test.cpp"));
    CHECK(benchmark->line == add_line);
    CHECK(benchmark->build.ndebug == find("registered by the macro")->build.ndebug);
    CHECK(benchmark->build.optimized == find("registered by the macro")->build.optimized);
}

TEST_CASE("benchmarks keep the order they were registered in") {
    const auto& benchmarks = bench::registry().benchmarks();
    REQUIRE(benchmarks.size() == 3);
    CHECK(benchmarks[0].name == "registered by the macro");
    CHECK(benchmarks[1].name == "registered by the long macro");
    CHECK(benchmarks[2].name == "registered by add()");
}

TEST_CASE("two benchmarks with one name throw std::logic_error naming both places") {
    bench::Registry registry;
    bench::Benchmark first = support::benchmark("sort", [] {});
    first.file = "a.cpp";
    first.line = 12;
    bench::Benchmark second = support::benchmark("sort", [] {});
    second.file = "b.cpp";
    second.line = 40;
    registry.add(first);
    CHECK_THROWS_AS(registry.add(second), std::logic_error);
    CHECK_THROWS_WITH(registry.add(second), "two benchmarks are named \"sort\": at a.cpp:12 and at b.cpp:40");
    CHECK(registry.benchmarks().size() == 1);
}

TEST_CASE("names differing only in case are different benchmarks") {
    bench::Registry registry;
    registry.add(support::benchmark("Sort", [] {}));
    registry.add(support::benchmark("sort", [] {}));
    CHECK(registry.benchmarks().size() == 2);
}
