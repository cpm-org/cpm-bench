// The JSON report (docs/spec.md §6.2), read back with cpm-json.
#include <cpm/bench.hpp>
#include <cpm/json.hpp>
#include <cpm/test.hpp>

#include <string>
#include <vector>

namespace bench = cpm::bench;
namespace json = cpm::json;

namespace {

bench::Result measured(std::string name) {
    bench::Result result;
    result.name = std::move(name);
    bench::Measurement measurement;
    measurement.time = bench::Statistics{412'000.0, 409'800.0, 431'200.0, 1.25};
    measurement.samples = 11;
    measurement.runs_per_sample = 32;
    result.measurement = measurement;
    return result;
}

json::Value report(const std::vector<bench::Result>& results) { return json::parse(bench::json_report(results)); }

}  // namespace

TEST_CASE("a measured benchmark has its numbers") {
    const json::Value document = report({measured("sort 10000")});
    const json::Value& entry = document.at("benchmarks").as_array().at(0);
    CHECK(entry.at("name").as_string() == "sort 10000");
    CHECK(entry.at("median_ns").get<double>() == 412'000.0);
    CHECK(entry.at("min_ns").get<double>() == 409'800.0);
    CHECK(entry.at("max_ns").get<double>() == 431'200.0);
    CHECK(entry.at("spread_percent").get<double>() == 1.25);
    CHECK(entry.at("samples").get<int>() == 11);
    CHECK(entry.at("runs_per_sample").get<int>() == 32);
    CHECK_FALSE(entry.as_object().contains("items_per_second"));
    CHECK_FALSE(entry.as_object().contains("bytes_per_second"));
    CHECK_FALSE(entry.as_object().contains("error"));
    CHECK(document.at("optimized").as_boolean());
}

TEST_CASE("throughput appears when the benchmark set it") {
    bench::Result result = measured("sort 10000");
    result.items = 10'000;
    result.bytes = 40'000;
    const json::Value document = report({result});
    const json::Value& entry = document.at("benchmarks").as_array().at(0);
    CHECK(entry.at("items_per_second").get<double>() == cpm::test::approx(24'271'844.66));
    CHECK(entry.at("bytes_per_second").get<double>() == cpm::test::approx(97'087'378.64));
}

TEST_CASE("a failed benchmark has its error instead of numbers") {
    bench::Result result;
    result.name = "parse";
    result.error = "unexpected end of input";
    const json::Value document = report({measured("sort"), result});
    const json::Value& entry = document.at("benchmarks").as_array().at(1);
    CHECK(entry.as_object().size() == 2);
    CHECK(entry.at("name").as_string() == "parse");
    CHECK(entry.at("error").as_string() == "unexpected end of input");
}

TEST_CASE("optimized is false when any benchmark's code wasn't") {
    bench::Result debug = measured("b");
    debug.build = bench::Build{true, false};
    CHECK_FALSE(report({measured("a"), debug}).at("optimized").as_boolean());
}

TEST_CASE("the members are in the spec's order, indented, ending in a line break") {
    const std::string text = bench::json_report(std::vector<bench::Result>{measured("x")});
    CHECK(text.ends_with("}\n"));
    CHECK(text.find("\n  \"benchmarks\": [") != std::string::npos);
    CHECK(text.find("\"name\"") < text.find("\"median_ns\""));
    CHECK(text.find("\"median_ns\"") < text.find("\"runs_per_sample\""));
    CHECK(text.find("\"benchmarks\"") < text.find("\"optimized\""));
}

TEST_CASE("no results make an empty list") {
    const json::Value document = report({});
    CHECK(document.at("benchmarks").as_array().empty());
    CHECK(document.at("optimized").as_boolean());
}
