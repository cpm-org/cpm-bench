// keep() and clobber() (docs/spec.md §3.2): they compile with values of every kind and change nothing.
#include <cpm/bench.hpp>
#include <cpm/test.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bench = cpm::bench;

namespace {

enum class Colour : std::uint8_t { red, green };

struct Point {
    double x;
    double y;
};

struct Big {
    std::array<std::uint64_t, 16> words{};
};

int answer() { return 42; }

}  // namespace

TEST_CASE("keep() takes numbers, enums and pointers, and leaves them as they were") {
    int i = 7;
    const double d = 2.5;
    long double ld = 1.25L;
    bool b = true;
    char c = 'x';
    Colour colour = Colour::green;
    int* pointer = &i;
    const char* text = "text";
    bench::keep(i);
    bench::keep(d);
    bench::keep(ld);
    bench::keep(b);
    bench::keep(c);
    bench::keep(colour);
    bench::keep(pointer);
    bench::keep(text);
    bench::keep(nullptr);
    CHECK(i == 7);
    CHECK(d == 2.5);
    CHECK(ld == 1.25L);
    CHECK(b);
    CHECK(c == 'x');
    CHECK(colour == Colour::green);
    CHECK(pointer == &i);
}

TEST_CASE("keep() takes structs, containers and temporaries") {
    Point point{1.0, 2.0};
    Big big;
    big.words[3] = 9;
    std::vector<int> numbers{1, 2, 3};
    std::string name = "a string long enough to be on the heap, not in the string itself";
    const std::string name_before = name;
    std::map<std::string, int> map{{"a", 1}};
    auto owned = std::make_unique<int>(5);
    bench::keep(point);
    bench::keep(big);
    bench::keep(numbers);
    bench::keep(name);
    bench::keep(map);
    bench::keep(owned);
    bench::keep(answer());
    bench::keep(std::vector<int>(100, 1));
    bench::keep(Point{3.0, 4.0});
    CHECK(point.x == 1.0);
    CHECK(big.words[3] == 9u);
    CHECK(numbers == std::vector<int>{1, 2, 3});
    CHECK(name == name_before);
    CHECK(map.at("a") == 1);
    CHECK(*owned == 5);
}

TEST_CASE("clobber() leaves memory as it was") {
    std::vector<int> numbers(1000, 3);
    numbers[500] = 4;
    bench::clobber();
    CHECK(numbers[500] == 4);
    CHECK(numbers[999] == 3);
}

TEST_CASE("keep() in a loop leaves the result as it was") {
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < 1000; ++i) {
        sum += i;
        bench::keep(sum);
    }
    bench::clobber();
    CHECK(sum == 499'500u);
}
