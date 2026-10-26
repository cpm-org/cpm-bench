#include "context.hpp"

#include <format>
#include <stdexcept>
#include <string_view>

namespace cpm::bench {
namespace detail {
namespace {

thread_local Context* current = nullptr;

Context& current_context(std::string_view function) {
    if (current == nullptr) throw std::logic_error(std::format("{} was called outside a benchmark", function));
    return *current;
}

}  // namespace

ContextScope::ContextScope(Context& context) noexcept : previous_(current) { current = &context; }

ContextScope::~ContextScope() { current = previous_; }

void measure(const Batch& batch) { current_context("measure()").measure(batch); }

}  // namespace detail

void items(std::uint64_t count) { detail::current_context("items()").items(count); }

void bytes(std::uint64_t count) { detail::current_context("bytes()").bytes(count); }

}  // namespace cpm::bench
