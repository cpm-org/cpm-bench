// Times, counts and rates as text (docs/spec.md §6.1): four significant digits, in units that step by 1000.
#pragma once

#include <string>

namespace cpm::bench::detail {

// `value` with four significant digits: 1.230, 12.30, 123.0, 1230 (and 12345 keeps all its digits). 0 for 0.
std::string significant(double value);

// 1.230 ns, 1.230 us, 1.230 ms, 1.230 s.
std::string format_time(double nanoseconds);

// 813.0, 813.0k, 813.0M, 813.0G.
std::string format_count(double count);

// 322.6 B/s, 322.6 KB/s, 322.6 MB/s, 322.6 GB/s.
std::string format_byte_rate(double bytes_per_second);

// 0.8%.
std::string format_percent(double percent);

}  // namespace cpm::bench::detail
