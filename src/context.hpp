// The benchmark running now, which measure(), items() and bytes() go to.
#pragma once

#include <cpm/bench/measure.hpp>

#include <cstdint>

namespace cpm::bench::detail {

class Context {
public:
    virtual ~Context() = default;

    virtual void measure(const Batch& batch) = 0;
    virtual void items(std::uint64_t count) = 0;
    virtual void bytes(std::uint64_t count) = 0;

protected:
    Context() = default;
    Context(const Context&) = default;
    Context& operator=(const Context&) = default;
};

// Makes `context` the current one on this thread while it lives.
class ContextScope {
public:
    explicit ContextScope(Context& context) noexcept;
    ~ContextScope();

    ContextScope(const ContextScope&) = delete;
    ContextScope& operator=(const ContextScope&) = delete;

private:
    Context* previous_;
};

}  // namespace cpm::bench::detail
