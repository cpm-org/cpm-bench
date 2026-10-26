// CPM_BENCH_NO_SHORT_MACROS leaves out BENCHMARK, for programs that have a macro of that name (docs/spec.md §3).
#define CPM_BENCH_NO_SHORT_MACROS
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#ifdef BENCHMARK
#error "BENCHMARK is defined although CPM_BENCH_NO_SHORT_MACROS is"
#endif

#define BENCHMARK(x) (x + 1)  // the program's own

CPM_BENCHMARK("long name only") {}

TEST_CASE("CPM_BENCHMARK works without the short macro, and the program's BENCHMARK is its own") {
    CHECK(cpm::bench::registry().benchmarks().size() == 1);
    CHECK(BENCHMARK(1) == 2);
}
