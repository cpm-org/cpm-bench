// Summarizing samples (docs/spec.md §4, step 4).
#include <statistics.hpp>

#include <cpm/test.hpp>

#include <vector>

using cpm::bench::Statistics;
using cpm::bench::detail::median;
using cpm::bench::detail::summarize;

TEST_CASE("the median of an odd count is the middle value") {
    CHECK(median({3.0}) == 3.0);
    CHECK(median({5.0, 1.0, 3.0}) == 3.0);
    CHECK(median({9.0, 2.0, 7.0, 4.0, 1.0}) == 4.0);
}

TEST_CASE("the median of an even count is the mean of the middle two") {
    CHECK(median({1.0, 2.0}) == 1.5);
    CHECK(median({8.0, 1.0, 4.0, 2.0}) == 3.0);
}

TEST_CASE("the median of nothing is 0") { CHECK(median({}) == 0.0); }

TEST_CASE("the extremes are the fastest and slowest samples") {
    const Statistics statistics = summarize({105.0, 98.0, 100.0, 131.0, 100.0});
    CHECK(statistics.min == 98.0);
    CHECK(statistics.max == 131.0);
    CHECK(statistics.median == 100.0);
}

TEST_CASE("the spread is the median absolute deviation as a percentage of the median") {
    // Deviations from 100: 5, 2, 0, 31, 0. Their median is 2, which is 2% of 100.
    CHECK(summarize({105.0, 98.0, 100.0, 131.0, 100.0}).spread == 2.0);
    // Deviations from 200: 20, 20, 0, 40, 10. Their median is 20: 10%.
    CHECK(summarize({180.0, 220.0, 200.0, 240.0, 190.0}).spread == 10.0);
    // An even count: median 15, deviations 5, 5, 5, 5.
    CHECK(summarize({10.0, 20.0, 10.0, 20.0}).spread == cpm::test::approx(100.0 / 3));
}

TEST_CASE("one slow sample moves neither the median nor the spread much") {
    const Statistics statistics = summarize({100.0, 101.0, 99.0, 100.0, 5000.0});
    CHECK(statistics.median == 100.0);
    CHECK(statistics.spread == 1.0);
    CHECK(statistics.max == 5000.0);
}

TEST_CASE("equal samples have no spread") {
    const Statistics statistics = summarize({7.0, 7.0, 7.0});
    CHECK(statistics.median == 7.0);
    CHECK(statistics.spread == 0.0);
}

TEST_CASE("a median of 0 has a spread of 0, not a division by 0") {
    CHECK(summarize({0.0, 0.0, 1.0}).spread == 0.0);
}

TEST_CASE("no samples summarize to zeros") {
    const Statistics statistics = summarize({});
    CHECK(statistics.median == 0.0);
    CHECK(statistics.min == 0.0);
    CHECK(statistics.max == 0.0);
    CHECK(statistics.spread == 0.0);
}
