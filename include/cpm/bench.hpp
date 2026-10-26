// cpm-bench: micro-benchmarks for C++20. See README.md for a guide and docs/spec.md for the specification.
//
//   #include <cpm/bench.hpp>
//
//   BENCHMARK("sort 10000") {
//       const std::vector<int> data = random_ints(10'000);  // setup, not timed
//       cpm::bench::items(data.size());                     // the report adds items per second
//       cpm::bench::measure([&] {                           // only this is timed, as often as needed
//           std::vector<int> copy = data;
//           std::sort(copy.begin(), copy.end());
//           cpm::bench::keep(copy);                         // so the compiler can't drop the work
//       });
//   }
//
//   CPM_BENCH_MAIN()
#pragma once

#include <cpm/bench/clock.hpp>
#include <cpm/bench/console_report.hpp>
#include <cpm/bench/json_report.hpp>
#include <cpm/bench/keep.hpp>
#include <cpm/bench/macros.hpp>
#include <cpm/bench/main.hpp>
#include <cpm/bench/measure.hpp>
#include <cpm/bench/registry.hpp>
#include <cpm/bench/result.hpp>
#include <cpm/bench/run.hpp>
