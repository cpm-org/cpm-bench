// Benchmarks of the words functions. `cpm test` runs each once to check it works; run the program built with
// --release to measure them:
//
//   cpm build --release --tests
//   build/release/tests/words_bench.test
#include <consumer/words.hpp>

#include <cpm/bench.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string sentence(std::size_t words) {
    std::string text;
    for (std::size_t i = 0; i < words; ++i) text += i % 3 == 0 ? "lorem " : "ipsum  ";
    return text;
}

}  // namespace

BENCHMARK("split 1000 words") {
    const std::string text = sentence(1000);
    cpm::bench::items(1000);
    cpm::bench::bytes(text.size());
    cpm::bench::measure([&] { return split_words(text); });
}

BENCHMARK("join 1000 words") {
    const std::string text = sentence(1000);
    const std::vector<std::string_view> words = split_words(text);
    cpm::bench::items(words.size());
    cpm::bench::measure([&] { return join_words(words); });
}

// Without measure(), the whole body is measured.
BENCHMARK("split a short line") {
    cpm::bench::keep(split_words("the quick brown fox"));
}

CPM_BENCH_MAIN()
