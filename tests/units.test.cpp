// Times, counts and rates as text (docs/spec.md §6.1), at every boundary.
#include <units.hpp>

#include <cpm/test.hpp>

using namespace cpm::bench::detail;

TEST_CASE("four significant digits") {
    CHECK(significant(1.23) == "1.230");
    CHECK(significant(12.3) == "12.30");
    CHECK(significant(123.0) == "123.0");
    CHECK(significant(1234.0) == "1234");
    CHECK(significant(0.312) == "0.3120");
    CHECK(significant(0.0004567) == "0.0004567");
    CHECK(significant(123456.0) == "123456");
    CHECK(significant(1.0) == "1.000");
    CHECK(significant(0.0) == "0");
}

TEST_CASE("rounding that adds a digit takes a decimal away") {
    CHECK(significant(9.9996) == "10.00");
    CHECK(significant(99.996) == "100.0");
    CHECK(significant(0.99996) == "1.000");
    CHECK(significant(9.9994) == "9.999");
}

TEST_CASE("times step through ns, us, ms and s") {
    CHECK(format_time(0.25) == "0.2500 ns");
    CHECK(format_time(1.0) == "1.000 ns");
    CHECK(format_time(999.9) == "999.9 ns");
    CHECK(format_time(999.96) == "1.000 us");
    CHECK(format_time(1000.0) == "1.000 us");
    CHECK(format_time(1230.0) == "1.230 us");
    CHECK(format_time(412'000.0) == "412.0 us");
    CHECK(format_time(999'940.0) == "999.9 us");
    CHECK(format_time(1e6) == "1.000 ms");
    CHECK(format_time(3.1e6) == "3.100 ms");
    CHECK(format_time(1e9) == "1.000 s");
    CHECK(format_time(61.5e9) == "61.50 s");
    CHECK(format_time(4321e9) == "4321 s");
    CHECK(format_time(0.0) == "0 ns");
}

TEST_CASE("counts step through k, M and G") {
    CHECK(format_count(322.58) == "322.6");
    CHECK(format_count(999.9) == "999.9");
    CHECK(format_count(1000.0) == "1.000k");
    CHECK(format_count(2427.18) == "2.427k");
    CHECK(format_count(813'008.13) == "813.0k");
    CHECK(format_count(24'271'844.66) == "24.27M");
    CHECK(format_count(1e9) == "1.000G");
    CHECK(format_count(2.5e12) == "2500G");
    CHECK(format_count(0.5) == "0.5000");
}

TEST_CASE("byte rates step through B/s, KB/s, MB/s and GB/s, in powers of 1000") {
    CHECK(format_byte_rate(512.0) == "512.0 B/s");
    CHECK(format_byte_rate(1000.0) == "1.000 KB/s");
    CHECK(format_byte_rate(1024.0) == "1.024 KB/s");
    CHECK(format_byte_rate(322'580'645.0) == "322.6 MB/s");
    CHECK(format_byte_rate(12.5e9) == "12.50 GB/s");
    CHECK(format_byte_rate(3e12) == "3000 GB/s");
}

TEST_CASE("percentages have one decimal") {
    CHECK(format_percent(0.8) == "0.8%");
    CHECK(format_percent(0.04) == "0.0%");
    CHECK(format_percent(6.06) == "6.1%");
    CHECK(format_percent(100.0) == "100.0%");
}
