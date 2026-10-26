#include "filter.hpp"

#include <algorithm>
#include <utility>

namespace cpm::bench::detail {
namespace {

bool matches_any(const std::vector<std::string>& patterns, std::string_view name) {
    return std::ranges::any_of(patterns, [&](const std::string& pattern) { return matches(pattern, name); });
}

bool matches_none(const std::string& pattern, const Registry& registry) {
    return std::ranges::none_of(registry.benchmarks(),
                                [&](const Benchmark& benchmark) { return matches(pattern, benchmark.name); });
}

}  // namespace

bool matches(std::string_view pattern, std::string_view text) noexcept {
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string_view::npos;  // the last '*' passed, to backtrack to
    std::size_t resume = 0;                      // where in text to try that '*' next
    while (t < text.size()) {
        if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            resume = t;
        } else if (p < pattern.size() && pattern[p] == text[t]) {
            ++p;
            ++t;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            t = ++resume;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

Filter::Filter(std::vector<std::string> include, std::vector<std::string> exclude)
    : include_(std::move(include)), exclude_(std::move(exclude)) {}

bool Filter::selects(std::string_view name) const {
    return (include_.empty() || matches_any(include_, name)) && !matches_any(exclude_, name);
}

std::vector<const Benchmark*> Filter::select(const Registry& registry) const {
    std::vector<const Benchmark*> chosen;
    for (const Benchmark& benchmark : registry.benchmarks()) {
        if (selects(benchmark.name)) chosen.push_back(&benchmark);
    }
    return chosen;
}

std::vector<std::string> Filter::unmatched(const Registry& registry) const {
    std::vector<std::string> patterns;
    for (const std::string& pattern : include_) {
        if (matches_none(pattern, registry)) patterns.push_back(pattern);
    }
    for (const std::string& pattern : exclude_) {
        if (matches_none(pattern, registry)) patterns.push_back('~' + pattern);
    }
    return patterns;
}

}  // namespace cpm::bench::detail
