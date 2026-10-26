// Benchmarks, and the registry BENCHMARK and add() put them in (docs/spec.md §3).
#pragma once

#include <functional>
#include <source_location>
#include <string>
#include <vector>

// How the code that includes this header is compiled. GCC and Clang define __OPTIMIZE__ when they optimize; other
// compilers have no such macro, so NDEBUG stands in for it.
#if defined(__OPTIMIZE__) || (!defined(__GNUC__) && !defined(__clang__) && defined(NDEBUG))
#define CPM_BENCH_OPTIMIZED_ true
#else
#define CPM_BENCH_OPTIMIZED_ false
#endif
#if defined(NDEBUG)
#define CPM_BENCH_NDEBUG_ true
#else
#define CPM_BENCH_NDEBUG_ false
#endif
#define CPM_BENCH_THIS_BUILD_ (::cpm::bench::Build{CPM_BENCH_OPTIMIZED_, CPM_BENCH_NDEBUG_})

namespace cpm::bench {

// How the code that defined a benchmark was compiled, for the warning about results from an unoptimized build.
struct Build {
    bool optimized = true;  // with optimization
    bool ndebug = true;     // with NDEBUG defined

    bool release() const noexcept { return optimized && ndebug; }
};

struct Benchmark {
    std::string name;
    std::function<void()> body;
    std::string file;
    int line = 0;
    Build build;
};

// The benchmarks, in the order they were registered. BENCHMARK and add() register in registry(); tests of the library
// build their own.
class Registry {
public:
    // Throws std::logic_error, naming both places, when a benchmark of that name is already registered.
    void add(Benchmark benchmark);

    const std::vector<Benchmark>& benchmarks() const noexcept { return benchmarks_; }

private:
    std::vector<Benchmark> benchmarks_;
};

// The registry BENCHMARK and add() register in.
Registry& registry();

// Registers a benchmark from code, such as one per size in a loop. Returns true, so it can initialize a variable at
// namespace scope. The default arguments are filled in where it is called, so they record the caller's build and place.
bool add(std::string name, std::function<void()> body, Build build = CPM_BENCH_THIS_BUILD_,
         std::source_location where = std::source_location::current());

namespace detail {

// What BENCHMARK runs while the program starts.
bool add_benchmark(std::string name, void (*body)(), const char* file, int line, Build build);

}  // namespace detail
}  // namespace cpm::bench
