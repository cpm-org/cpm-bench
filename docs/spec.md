# cpm-bench 0.1.0: specification

cpm-bench measures how long C++20 code takes. A benchmark is a block of code that it runs many times, in batches long enough for the clock to measure precisely. It reports the median time per run with its spread, and the throughput when a benchmark says how much it processes. Under `cpm test`, the same benchmarks run once each, as a check that they still work. It is a [cpm-org](https://github.com/cpm-org) library: original code with its own API, built and tested with [CPM](https://github.com/meirborowski/CPM). This document is what the code is written against. The README will be the user's guide.

## 1. Scope

0.1.0 covers:

- **Benchmarks** defined with a macro, or registered with a function, with optional setup that isn't timed.
- **Measurement:** calibrated batches, a warm-up, several samples, and the median, minimum, maximum and spread per run.
- **Throughput** in items or bytes per second.
- **Keeping the compiler honest:** `keep(value)` and `clobber()`, so the code measured isn't optimized away.
- **A runner:** a `main()` defined with one macro, with filtering by pattern, the number of samples and the time per sample, a list of the benchmarks, and a check mode for `cpm test`.
- **Reports:** an aligned table on the console, and JSON for tools that track results over time.
- **Warnings** about results that can't be trusted: an unoptimized build, a spread above 5%, and a benchmark too fast to measure.

Not in 0.1.0:

- Comparing results with an earlier run.
- Ranges of parameters, and estimating complexity.
- Hardware performance counters.
- Pinning threads to processors, and statistics across processes.
- Benchmarks that run on several threads at once.
- Charts.

## 2. The package

| | |
|---|---|
| Repository | `github.com/cpm-org/cpm-bench`, MIT |
| Registry name | `cpm-bench` |
| Header | `<cpm/bench.hpp>`, which includes the parts in `<cpm/bench/*.hpp>` |
| Namespace | `cpm::bench` |
| Standard | `"std": "c++20"` |
| Type | `static-lib` |
| Dependencies | [cpm-args](https://github.com/cpm-org/cpm-args) for the runner's command line, and [cpm-json](https://github.com/cpm-org/cpm-json) for the JSON report, both `^0.1.0` from cpm-registry.com |
| Tests | `tests/*.test.cpp` on cpm-test, run with `cpm test` |
| CI | Linux (GCC and Clang), macOS (Apple Clang) and Windows (MinGW GCC), with CPM installed by its installer: strict warnings as errors, release, and AddressSanitizer with UBSan on Linux |

cpm-bench is the first cpm-org library with dependencies of its own. Installing it brings cpm-args and cpm-json from the registry, which tests CPM's handling of registry dependencies of a registry package.

## 3. Defining benchmarks

```cpp
#include <cpm/bench.hpp>

// The whole body is measured.
BENCHMARK("vector push_back 1000") {
    std::vector<int> v;
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    cpm::bench::keep(v);
}

// Setup isn't measured: only what measure() runs is.
BENCHMARK("sort 10000") {
    const std::vector<int> data = random_ints(10'000);
    cpm::bench::items(data.size());
    cpm::bench::measure([&] {
        std::vector<int> copy = data;
        std::sort(copy.begin(), copy.end());
        return copy;  // kept
    });
}

CPM_BENCH_MAIN()
```

- `BENCHMARK(name)` defines and registers a benchmark. The name is any text, and names must be unique.
- **Without `measure()`**, the body is the code measured: it runs once to warm up, then as often as the measurement needs.
- **With `measure(f)`**, the body runs once, and the measurement runs `f` as often as it needs. Code before and after the call is setup and teardown, and isn't timed. A value `f` returns is passed to `keep()`. A body calls `measure` at most once; a second call is an error (§7).
- `cpm::bench::add(name, function)` registers a benchmark from code, such as one per size in a loop. It returns `true`, so it can initialize a variable at namespace scope.
- `CPM_BENCHMARK` is the long name of the macro. Defining `CPM_BENCH_NO_SHORT_MACROS` before the header leaves out `BENCHMARK`, for programs that already have a macro of that name.
- The macro records whether the code around it was optimized and whether `NDEBUG` was defined, for the warnings (§6). Optimization is `__OPTIMIZE__` on GCC and Clang; other compilers have no such macro, so `NDEBUG` stands in for it. `add()` records the same through default arguments, which are filled in where it is called.

### 3.1 Throughput

Called in a benchmark's body:
- `items(n)`: each run processes `n` items. The report adds items per second.
- `bytes(n)`: each run processes `n` bytes. The report adds bytes per second.

`measure()`, `items()` and `bytes()` throw `std::logic_error` outside a benchmark.

### 3.2 Keeping the compiler honest

- `keep(value)` makes the compiler assume that `value` is read, so it can't drop the code that computed it.
- `clobber()` makes it assume that all memory is read and written, so it can't drop stores or move them across the call.
- With GCC and Clang both are empty `asm volatile` statements. `keep` reads a number, enum or pointer where it is, in a register or in memory, and anything else through its address with a memory clobber, so the memory a container owns counts as read too. Elsewhere, `keep` writes the value's address to a `volatile` pointer and `clobber` is a compiler fence.
- Neither has a cost beyond what it prevents.

## 4. Measuring

For each benchmark:

1. **Calibrate.** Run the measured code in batches of 1, 2, 4, … runs until a batch takes at least the time per sample (default 10 ms). That batch size is used for every sample. One run alone taking longer is a batch of 1. Doubling stops at 2⁴⁰ runs, since code the compiler removed takes no time at any size.
2. **Warm up.** Run one batch, without recording it.
3. **Sample.** Run `samples` batches (default 11), timing each with `std::chrono::steady_clock`. Each sample's time per run is the batch's time divided by its size.
4. **Summarize:**
   - the median time per run, and the fastest and slowest samples;
   - the spread: the median absolute deviation from the median, as a percentage of the median;
   - throughput, from the median.

The numbers are wall-clock time on one thread. A benchmark that throws is reported as failed with its message, and the others still run.

## 5. The runner

A benchmark program defines its `main()` with `CPM_BENCH_MAIN()`, or calls `cpm::bench::main(argc, argv)` from a `main()` of its own. The library has no `main()`: a project's test programs often link cpm-test too, and with a `main()` in each library the linker could take either. Like a test program's, the exit status is 0 when everything ran, 1 when a benchmark failed or the JSON file couldn't be written, and 2 for a bad command line.

```
Runs the benchmarks whose names match a pattern, or all of them.

Usage: words_bench.test [OPTIONS] [PATTERN...]

Arguments:
  [PATTERN...]  * matches any text; a leading ~ leaves out what it matches

Options:
  -l, --list              list the benchmarks instead of running them
      --samples <N>       samples per benchmark (default: 11)
      --sample-time <MS>  the least time per sample, in ms (default: 10)
      --json <FILE>       also write the results to FILE as JSON
      --check             run each benchmark once, unmeasured [env: CPM_TEST]
      --measure           measure, even with --check or under cpm test
  -h, --help              show this help
```

- The command line is read with cpm-args, which writes the help and the errors. The program's name is argv[0] without its directory and `.exe`.
- A pattern that matches no benchmark, included or left out, is a bad command line, so a typo doesn't pass silently: `error: no benchmark matches 'prase'; --list shows their names`.
- **Check mode** runs each benchmark's code once, including what it passes to `measure()`. It reports only failures (`parse: failed: <message>`), then a count: `checked 3 benchmarks`, or `checked 3 benchmarks: 1 failed`.
  - It is on with `--check`, and when the environment variable `CPM_TEST` is set, which `cpm test` does (cpm-args reads the variable as a boolean). `--measure` turns it off.
  - So benchmarks kept in `tests/*.test.cpp` are checked by `cpm test` in a fraction of a second, and measured when their program is run directly. `cpm build --release --tests` builds them optimized.
  - On Windows, a program run directly finds the compiler's runtime DLLs only through `PATH`, which `cpm test` sets and a shell may not. The example project's release profile sets `"runtime": "static"`, and the README recommends it.

## 6. Reports

### 6.1 The console

```
benchmark               time/run     spread   runs/s     throughput
vector push_back 1000   1.230 us     0.8%     813.0k
sort 10000              412.0 us     1.2%     2.427k     24.27M items/s
parse 1 MB              3.100 ms     6.1% !   322.6      322.6 MB/s
tiny                    2.500 ns *   0.4%     400.0M
broken                  failed: no input

these results are from code built without optimization; build with --release
! the spread is above 5%: the machine may have been busy
* too fast to measure reliably: under 10.00 ns a run, 10 times the clock's resolution
```

- A row appears as each benchmark finishes, so a slow one shows the results before it.
- Times use `ns`, `us`, `ms` and `s` with four significant digits; counts use `k`, `M` and `G`; bytes per second use `B/s`, `KB/s`, `MB/s` and `GB/s` in powers of 1000. A benchmark with both items and bytes shows both rates, separated by a comma.
- The name column is the longest name (or `benchmark`) plus three spaces. The time column is 13 characters, the spread 9 and runs per second 11, room for a mark after the time and the spread. Lines have no trailing spaces.
- Failures appear in place of results: `failed: <message>`.
- **Warnings** come below the table, after a blank line, in this order:
  - when any benchmark was compiled without optimization or without `NDEBUG`: `these results are from code built without optimization; build with --release`;
  - when a spread is above 5%, which marks it with `!`: as in the example;
  - when a run takes less than 10 times the clock's resolution, which marks the time with `*`. The resolution is the clock's tick: 1 ns for `steady_clock` on the usual platforms.
- With no benchmarks to run, the report is `no benchmarks to run`.

### 6.2 JSON

`--json FILE` writes, with cpm-json:

```json
{
  "benchmarks": [
    {
      "name": "sort 10000",
      "median_ns": 412000.0,
      "min_ns": 409800.0,
      "max_ns": 431200.0,
      "spread_percent": 1.2,
      "samples": 11,
      "runs_per_sample": 32,
      "items_per_second": 24271844.7
    }
  ],
  "optimized": true
}
```

- `items_per_second` and `bytes_per_second` appear only when the benchmark set them.
- A failed benchmark has `"error"` instead of the numbers. In check mode, entries have only their names (and errors).
- `optimized` is false when any benchmark was compiled without optimization or without `NDEBUG`.
- The document is indented by two spaces and ends in a line break.

## 7. Errors

- Two benchmarks with the same name throw `std::logic_error` when the second registers. The exception escapes before `main()`, and the message names both places: `two benchmarks are named "sort": at a.cpp:12 and at b.cpp:40`.
- `measure()` called twice in one body fails that benchmark: `measure() was called twice in one benchmark`. So does a body that calls it on a later run but not on the first.
- An exception that isn't a `std::exception` fails the benchmark with `an exception that isn't a std::exception`.
- `measure()`, `items()` and `bytes()` outside a benchmark throw `std::logic_error`: `measure() was called outside a benchmark`.
- A JSON file that can't be written is a runner error: `error: couldn't write the results to FILE` goes to standard error, with exit status 1.

## 8. Structure

Each part has one job and sits behind a small interface. Time comes from a `Clock` interface, so calibration, sampling and reports are tested with a fake clock that advances exactly as a test says.

| Part | Job |
|---|---|
| `keep` | `keep` and `clobber`, in the header |
| `macros` | `BENCHMARK`, `CPM_BENCHMARK` and `CPM_BENCH_MAIN`, in the header |
| `registry` | the registered benchmarks, with their source locations and build flags |
| `context` | the benchmark running now, for `measure`, `items` and `bytes` |
| `clock` | `Clock`, and the `steady_clock` implementation |
| `calibration` | the batch size for a sample time |
| `sampler` | warm-up and samples of a measured function |
| `runner` | runs each selected benchmark: with or without `measure()`, in measuring or check mode, and catching failures; tells a `Reporter` |
| `statistics` | median, extremes and spread |
| `result` | a benchmark's `Result`, and what it says: rates, a high spread, too fast to measure |
| `units` | times, counts and rates as text |
| `filter` | patterns with `*` and `~` |
| `console_report`, `json_report` | the reports |
| `command_line` | the options, through cpm-args |
| `main` | `cpm::bench::main`: the command line, choosing, running and the reports, with the environment, clock and streams passed in for tests |

The public headers are those a user or a tool built on cpm-bench needs: `keep`, `macros`, `registry`, `measure` (the context's functions), `clock`, `result`, `run` (settings, `Reporter` and `run()`), the two reports and `main`. Calibration, sampling, statistics, units, patterns and the command line stay in `src/`. Neither cpm-args nor cpm-json appears in a public header.

## 9. Testing

- **Statistics:** medians of odd and even counts, the spread, and extremes.
- **Calibration and sampling** with a fake clock:
  - the batch sizes chosen;
  - one slow run;
  - the warm-up isn't recorded;
  - setup in a body that uses `measure()` runs once.
- **Units** at every boundary.
- **Patterns:** wildcards, exclusions, and a pattern that matches nothing.
- **Reports:** the console table compared exactly with a fake clock, including each warning. The JSON report is read back with cpm-json.
- **Check mode:** with `CPM_TEST` and `--check`, and turned off with `--measure`.
- **The command line:** the help compared exactly, `--list`, patterns, bad values, an unknown option, and the JSON file, including one that can't be written.
- **Failures:** a throwing benchmark, `measure()` called twice, and duplicate names.
- **The real clock:** a smoke test measures a real loop with a short sample time and checks that the numbers are positive and consistent.
- **`keep` and `clobber`:** they compile with values of every kind and change nothing.
- **Example project:** `examples/consumer` has benchmarks in `tests/` next to unit tests on cpm-test. CI runs both with `cpm test` (the benchmarks in check mode), then measures the benchmarks by running their program built with `--release`.
