// Unit tests, on cpm-test, next to the benchmarks in words_bench.test.cpp.
#include <consumer/words.hpp>

#include <cpm/test.hpp>

TEST_CASE("words are what is between runs of spaces") {
    CHECK(split_words("  the quick  brown fox ") == (std::vector<std::string_view>{"the", "quick", "brown", "fox"}));
    CHECK(split_words("   ").empty());
}

TEST_CASE("joining puts one space between words") {
    CHECK(join_words(split_words("  the quick  brown fox ")) == "the quick brown fox");
    CHECK(join_words({}).empty());
}
