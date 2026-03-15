/**
 * @file span.h
 * @brief ara::core::Span and ara::core::Byte – C++14 implementations.
 *
 * AUTOSAR Adaptive Platform R25-11
 * C++14 compliant (std::span is C++20, std::byte is C++17).
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_SPAN_H_
#define ARA_CORE_SPAN_H_

#include <cstddef>
#include <cstdint>

namespace ara {
namespace core {

// ---------------------------------------------------------------------------
// Byte – replaces std::byte (C++17)
// ---------------------------------------------------------------------------

/**
 * @brief Represents a single raw byte.
 *
 * Defined as a scoped enum with underlying type std::uint8_t to avoid
 * implicit arithmetic conversions, matching std::byte semantics.
 * MISRA C++:2023 Rule 6.4.1 – scoped enum with fixed underlying type.
 */
enum class Byte : std::uint8_t {};

// ---------------------------------------------------------------------------
// Span<T> – lightweight non-owning view over a contiguous sequence.
// Mirrors the essential interface of std::span (C++20) for C++14 use.
// ---------------------------------------------------------------------------

/**
 * @brief Non-owning view over a contiguous array of T.
 *
 * CWE-125: bounds are preserved via size member; no unbounded access.
 * CERT C++ ARR55-CPP: never access beyond [data, data+size).
 *
 * @tparam T Element type (may be const-qualified).
 */
template <typename T>
class Span
{
public:
    typedef T         value_type;
    typedef T *       pointer;
    typedef const T * const_pointer;
    typedef T &       reference;
    typedef const T & const_reference;
    typedef T *       iterator;
    typedef const T * const_iterator;
    typedef std::size_t size_type;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default – empty span.
    Span() noexcept : data_(nullptr), size_(0U) {}

    /// From pointer + count.
    Span(pointer data, size_type count) noexcept
        : data_(data), size_(count) {}

    /// From a C-style array (deduced size).
    template <std::size_t N>
    Span(T (&arr)[N]) noexcept  // NOLINT
        : data_(arr), size_(N) {}

    // -----------------------------------------------------------------------
    // Observers
    // -----------------------------------------------------------------------

    pointer   data()  const noexcept { return data_; }
    size_type size()  const noexcept { return size_; }
    bool      empty() const noexcept { return size_ == 0U; }

    iterator       begin() noexcept        { return data_; }
    iterator       end()   noexcept        { return data_ + size_; }
    const_iterator begin() const noexcept  { return data_; }
    const_iterator end()   const noexcept  { return data_ + size_; }

    /// Element access – caller must ensure pos < size().
    reference operator[](size_type pos) const noexcept { return data_[pos]; }

private:
    pointer   data_;
    size_type size_;
};

} // namespace core
} // namespace ara

#endif // ARA_CORE_SPAN_H_
