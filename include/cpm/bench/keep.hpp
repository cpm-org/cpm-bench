// Keeping the compiler honest (docs/spec.md §3.2): code whose results nothing uses can be optimized away, and then a
// benchmark measures nothing.
#pragma once

#include <memory>
#include <type_traits>

#if !defined(__GNUC__) && !defined(__clang__)
#include <atomic>
#endif

namespace cpm::bench {

#if defined(__GNUC__) || defined(__clang__)

namespace detail {
// What keep() reads where it is, in a register or in memory, rather than through its address.
template <class T>
concept scalar_in_register =
    (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>) && sizeof(T) <= sizeof(void*);
}  // namespace detail

// Makes the compiler assume that `value` is read, so it can't drop the code that computed it. A number, enum or pointer
// is read where it is; anything else through its address, with all the memory it owns.
template <class T>
inline void keep(const T& value) noexcept {
    if constexpr (detail::scalar_in_register<T>) {
        __asm__ __volatile__("" : : "r,m"(value));
    } else {
        __asm__ __volatile__("" : : "r"(std::addressof(value)) : "memory");
    }
}

// Makes the compiler assume that all memory is read and written here, so it can't drop stores or move them across
// the call.
inline void clobber() noexcept { __asm__ __volatile__("" : : : "memory"); }

#else

namespace detail {
// Where keep() writes addresses: a volatile store the compiler must make, of an address that then escapes.
inline const volatile void* volatile kept = nullptr;
}  // namespace detail

template <class T>
inline void keep(const T& value) noexcept {
    detail::kept = std::addressof(value);
}

inline void clobber() noexcept { std::atomic_signal_fence(std::memory_order_seq_cst); }

#endif

}  // namespace cpm::bench
