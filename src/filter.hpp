// Choosing benchmarks by name (docs/spec.md §5).
#pragma once

#include <cpm/bench/registry.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cpm::bench::detail {

// Whether `text` matches `pattern`, in which `*` matches any text and every other character itself.
bool matches(std::string_view pattern, std::string_view text) noexcept;

// The benchmarks matching an include pattern (all of them when there are none), minus those matching an exclude
// pattern.
class Filter {
public:
    Filter(std::vector<std::string> include, std::vector<std::string> exclude);

    bool selects(std::string_view name) const;
    std::vector<const Benchmark*> select(const Registry& registry) const;

    // The patterns no benchmark in `registry` matches, as written on the command line (exclusions with their ~).
    // They are more likely typos than wishes to run nothing.
    std::vector<std::string> unmatched(const Registry& registry) const;

private:
    std::vector<std::string> include_;
    std::vector<std::string> exclude_;
};

}  // namespace cpm::bench::detail
