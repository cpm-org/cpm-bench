// The macros benchmarks are defined with. BENCHMARK is defined too unless CPM_BENCH_NO_SHORT_MACROS is.
#pragma once

#include <cpm/bench/main.hpp>
#include <cpm/bench/registry.hpp>

#define CPM_BENCH_CAT2_(a, b) a##b
#define CPM_BENCH_CAT_(a, b) CPM_BENCH_CAT2_(a, b)
#define CPM_BENCH_UNIQUE_(prefix) CPM_BENCH_CAT_(prefix, __COUNTER__)

// CPM_BENCHMARK("name") { body }: defines a benchmark and registers it while the program starts, with its place and
// how the code around it is compiled.
#define CPM_BENCHMARK(name) CPM_BENCHMARK_IMPL_(CPM_BENCH_UNIQUE_(cpm_bench_body_), name)
#define CPM_BENCHMARK_IMPL_(fn, name)                                                                            \
    static void fn();                                                                                            \
    [[maybe_unused]] static const bool CPM_BENCH_CAT_(fn, _registered) =                                         \
        ::cpm::bench::detail::add_benchmark(name, &fn, __FILE__, __LINE__, CPM_BENCH_THIS_BUILD_);               \
    static void fn()

// The main() of a benchmark program. The library has none of its own: a project's test programs link cpm-test too,
// and the linker could take a main() from either library.
#define CPM_BENCH_MAIN() \
    int main(int argc, char* argv[]) { return ::cpm::bench::main(argc, argv); }

#ifndef CPM_BENCH_NO_SHORT_MACROS
#define BENCHMARK(name) CPM_BENCHMARK(name)
#endif
