# cpm-bench

Micro-benchmarks for C++20, made for [CPM](https://github.com/meirborowski/CPM). cpm-bench runs your code in batches long enough to time precisely, and reports the median time per run, how much it varied, and the throughput. Under `cpm test` the same benchmarks run once each, so they are checked along with your tests.

```sh
cpm install -D cpm-bench
```

Put benchmarks in `tests/`, next to your tests. Here is `tests/words_bench.test.cpp`:

```cpp
#include <consumer/words.hpp>

#include <cpm/bench.hpp>

#include <string>

namespace {

std::string sentence(std::size_t words) {
    std::string text;
    for (std::size_t i = 0; i < words; ++i) text += i % 3 == 0 ? "lorem " : "ipsum  ";
    return text;
}

}  // namespace

// Setup isn't timed: only the code given to measure() is.
BENCHMARK("split 1000 words") {
    const std::string text = sentence(1000);
    cpm::bench::items(1000);           // the report adds items per second
    cpm::bench::bytes(text.size());    // and bytes per second
    cpm::bench::measure([&] { return split_words(text); });
}

// Without measure(), the whole body is timed.
BENCHMARK("split a short line") {
    cpm::bench::keep(split_words("the quick brown fox"));  // so the compiler can't drop the work
}

CPM_BENCH_MAIN()
```

`cpm test` checks that each benchmark runs. To measure, build the tests optimized and run the program:

```
$ cpm build --release --tests
$ build/release/tests/words_bench.test
benchmark            time/run     spread   runs/s     throughput
split 1000 words     7.838 us     0.4%     127.6k     127.6M items/s, 850.5 MB/s
split a short line   133.9 ns     0.7%     7.469M
```

The full example is in [examples/consumer](examples/consumer).

## Benchmarks

`BENCHMARK("name") { ... }` defines a benchmark and registers it while the program starts. The name can be any text, and names must be unique. Two benchmarks with the same name throw `std::logic_error` before `main()` runs, and the message names both places.

- **Without `measure()`,** the body is the code timed. It runs once to warm up, then as often as the measurement needs.
- **With `cpm::bench::measure(f)`,** the body runs once and `f` runs as often as needed. The body's code before and after the call is setup and teardown, and isn't timed. If `f` returns a value, the value is passed to `keep()`. A body calls `measure()` at most once.
- **Throughput:** `cpm::bench::items(n)` says each run processes `n` items, and `cpm::bench::bytes(n)` says it processes `n` bytes. The report then shows items or bytes per second.

`measure()`, `items()` and `bytes()` throw `std::logic_error` when they are called outside a benchmark.

To register benchmarks from code, for example one per size, use `cpm::bench::add(name, function)`. It returns `true`, so it can initialize a variable:

```cpp
#include <cpm/bench.hpp>

#include <algorithm>
#include <format>
#include <random>
#include <vector>

std::vector<int> random_ints(std::size_t count) {
    std::mt19937 random(42);
    std::vector<int> values(count);
    for (int& value : values) value = static_cast<int>(random());
    return values;
}

constexpr std::size_t sizes[] = {100, 10'000, 1'000'000};

const bool sorts = [] {
    for (const std::size_t size : sizes) {
        cpm::bench::add(std::format("sort {}", size), [size] {
            const std::vector<int> data = random_ints(size);
            cpm::bench::items(size);
            cpm::bench::measure([&] {
                std::vector<int> copy = data;
                std::ranges::sort(copy);
                return copy;
            });
        });
    }
    return true;
}();

CPM_BENCH_MAIN()
```

`CPM_BENCHMARK` is the long name of `BENCHMARK`. Define `CPM_BENCH_NO_SHORT_MACROS` before including the header to leave out `BENCHMARK`, if your program has its own macro of that name.

## Keeping the compiler honest

An optimizer removes code whose results are never used, and then a benchmark measures nothing.

- `cpm::bench::keep(value)` makes the compiler assume that `value` is read, so the code that computed it stays. For a container, this includes the memory the container owns.
- `cpm::bench::clobber()` makes the compiler assume that all memory is read and written at that point, so stores can't be dropped or moved across it.

With GCC and Clang, both are empty `asm` statements: they cost nothing beyond the work they keep. A value that `measure()`'s function returns is kept for you.

## How it measures

1. **Calibrate.** Run the code in batches of 1, 2, 4, … runs until a batch takes at least the time per sample (10 ms by default). That batch size is used for every sample.
2. **Warm up.** Run one batch, and don't record it.
3. **Sample.** Time 11 batches with `std::chrono::steady_clock`. Each sample's time per run is the batch's time divided by its size.
4. **Summarize.** Report the median time per run, the spread, and the throughput from the median. The spread is the median absolute deviation from the median, as a percentage of the median. The JSON report also has the fastest and slowest samples.

The times are wall-clock time on one thread. A benchmark that throws is reported as failed with its message, and the others still run.

## Running benchmarks

```
$ build/release/tests/words_bench.test --help
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

- **Patterns:** `words_bench.test "split*" "~*short*"` runs the benchmarks whose names start with `split`, except those containing `short`. A pattern that matches no benchmark is an error, so a typo doesn't pass unnoticed.
- **Check mode** runs each benchmark's code once, including what it gives `measure()`, and reports only failures and a count. `cpm test` sets `CPM_TEST`, which turns check mode on; `--check` does too, and `--measure` turns it off.
- **Exit status:** 0 when every benchmark ran, 1 when one failed or the JSON file couldn't be written, and 2 for a bad command line.
- **On Windows,** a program run from a shell finds the compiler's runtime DLLs only when the compiler's directory is on `PATH`. `cpm test` puts it there, but the compiler that `cpm setup --managed` installs isn't on your `PATH`. Add `"profiles": { "release": { "runtime": "static" } }` to your manifest so the program carries its runtime with it, as [examples/consumer](examples/consumer/cpm.jsonc) does.

A program that needs its own `main()`, for setup, calls `cpm::bench::main(argc, argv)` instead of using `CPM_BENCH_MAIN()`. The library has no `main()` of its own. A project's test programs often link cpm-test too, and the linker could then take `main()` from the wrong library.

## Reading the report

```
benchmark               time/run     spread   runs/s     throughput
vector push_back 1000   1.230 us     0.8%     813.0k
sort 10000              412.0 us     1.2%     2.427k     24.27M items/s
parse 1 MB              3.100 ms     6.1% !   322.6      322.6 MB/s
tiny                    2.500 ns *   0.4%     400.0M

these results are from code built without optimization; build with --release
! the spread is above 5%: the machine may have been busy
* too fast to measure reliably: under 10.00 ns a run, 10 times the clock's resolution
```

- Rows appear as each benchmark finishes. A failed benchmark shows `failed:` and its message in place of the numbers.
- Times use `ns`, `us`, `ms` and `s`, counts use `k`, `M` and `G`, and byte rates use `KB/s`, `MB/s` and `GB/s` (powers of 1000), all with four significant digits.
- The warnings say when not to trust the numbers:
  - The code wasn't optimized, or was built without `NDEBUG`. `BENCHMARK` and `add()` record how the code around them was compiled.
  - The spread is above 5%: other work on the machine probably got in the way. Run again, or close other programs.
  - A run takes less than 10 ticks of the clock. At that scale, the loop around the code is a large part of the time. Usually the result wasn't kept and the compiler removed the work.

`--json FILE` also writes the results as JSON, for tools that track them over time:

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
      "items_per_second": 24271844.66
    }
  ],
  "optimized": true
}
```

- `items_per_second` and `bytes_per_second` appear only when the benchmark set them.
- A failed benchmark has `"error"` instead of the numbers. In check mode, the entries have only names and errors.
- `optimized` is false when any benchmark's code wasn't optimized or was built without `NDEBUG`.

## Building on it

The parts that `main()` puts together are public:

- `cpm::bench::Registry` holds benchmarks.
- `cpm::bench::run()` runs them and returns a `Result` for each one, telling a `Reporter` as it goes.
- `ConsoleReporter` and `json_report()` write the reports.
- Time comes from a `cpm::bench::Clock`. A test can pass a clock that advances exactly as the test says, as cpm-bench's own tests do.

## Platforms

cpm-bench is tested on:

- Linux, with GCC and Clang, including AddressSanitizer and UBSan;
- macOS, with Apple Clang;
- Windows, with MinGW GCC.

It uses [cpm-args](https://github.com/cpm-org/cpm-args) for its command line and [cpm-json](https://github.com/cpm-org/cpm-json) for the JSON report, both installed with it.
