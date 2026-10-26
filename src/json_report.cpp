#include <cpm/bench/json_report.hpp>
#include <cpm/json.hpp>

#include <algorithm>

namespace cpm::bench {
namespace {

namespace json = cpm::json;

json::Value entry(const Result& result) {
    json::Object object{{"name", result.name}};
    if (result.error) {
        object.insert_or_assign("error", *result.error);
    } else if (const auto& measurement = result.measurement) {
        object.insert_or_assign("median_ns", measurement->time.median);
        object.insert_or_assign("min_ns", measurement->time.min);
        object.insert_or_assign("max_ns", measurement->time.max);
        object.insert_or_assign("spread_percent", measurement->time.spread);
        object.insert_or_assign("samples", measurement->samples);
        object.insert_or_assign("runs_per_sample", measurement->runs_per_sample);
        if (const auto items = items_per_second(result)) object.insert_or_assign("items_per_second", *items);
        if (const auto bytes = bytes_per_second(result)) object.insert_or_assign("bytes_per_second", *bytes);
    }
    return object;
}

}  // namespace

std::string json_report(std::span<const Result> results) {
    json::Array benchmarks;
    benchmarks.reserve(results.size());
    for (const Result& result : results) benchmarks.push_back(entry(result));
    const bool optimized = std::ranges::all_of(results, [](const Result& result) { return result.build.release(); });
    const json::Value document = json::Object{{"benchmarks", std::move(benchmarks)}, {"optimized", optimized}};
    return json::write(document, {.indent = 2}) + '\n';
}

}  // namespace cpm::bench
