# Changelog

## 0.1.0

The first release.

- **Benchmarks** defined with `BENCHMARK` or registered with `add()`. Setup and teardown around `measure()` isn't timed, and without `measure()` the whole body is.
- **Measurement:** calibrated batches, a warm-up, and samples summarized as the median time per run, the extremes, and the spread (the median absolute deviation).
- **Throughput** in items or bytes per second.
- **`keep()` and `clobber()`,** so the code measured isn't optimized away.
- **A runner** for `CPM_BENCH_MAIN()` or your own `main()`. It filters by pattern, sets the number of samples and the time per sample, and lists the benchmarks. A check mode runs each benchmark once under `cpm test`.
- **Reports:** an aligned table on the console, with warnings about an unoptimized build, a high spread, and runs too fast to measure. A JSON file is written with cpm-json.
- **Errors:** duplicate names throw before `main()`. A benchmark that throws fails alone, and a pattern that matches nothing is a bad command line.
