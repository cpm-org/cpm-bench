// What a benchmark's body calls (docs/spec.md §3): measure() to say which code is measured, items() and bytes() for
// throughput. They go to the benchmark running now, and throw std::logic_error outside one.
#pragma once

#include <cpm/bench/keep.hpp>

#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace cpm::bench {
namespace detail {

// Runs the code measured `runs` times in a row. The loop is in the caller's code, so the code measured can be inlined
// into it; only the call of a whole batch is indirect.
using Batch = std::function<void(std::uint64_t runs)>;

void measure(const Batch& batch);

}  // namespace detail

// The code to measure, instead of the whole body: the body runs once, and `function` as often as the measurement
// needs. What the body does before and after is setup and teardown, which isn't timed. A value `function` returns is
// kept (see keep()). A body calls measure() at most once.
template <class F>
    requires std::invocable<F&>
void measure(F&& function) {
    detail::measure([&function](std::uint64_t runs) {
        for (std::uint64_t run = 0; run < runs; ++run) {
            if constexpr (std::is_void_v<std::invoke_result_t<F&>>) {
                function();
            } else {
                keep(function());
            }
        }
    });
}

// Each run processes `count` items: the report adds items per second.
void items(std::uint64_t count);

// Each run processes `count` bytes: the report adds bytes per second.
void bytes(std::uint64_t count);

}  // namespace cpm::bench
