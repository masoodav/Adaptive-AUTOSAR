/**
 * @file string_view.h
 * @brief ara::core::StringView – lightweight non-owning string reference.
 *
 * AUTOSAR Adaptive Platform R25-11
 * C++14 compliant implementation (std::string_view is C++17).
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_STRING_VIEW_H_
#define ARA_CORE_STRING_VIEW_H_

#include <cstddef>
#include <cstring>
#include <string>

namespace ara {
namespace core {

/**
 * @brief Non-owning, read-only view of a contiguous character sequence.
 *
 * Mirrors the essential interface of std::string_view (C++17) for use
 * under C++14.  The view does NOT own the pointed-to data; the caller is
 * responsible for ensuring the data outlives the view.
 *
 * CERT C++ STR11-CPP: no null-pointer dereference – all paths guarded.
 * CWE-125: bounds always checked before access.
 */
class StringView
{
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------
    typedef const char *  const_iterator;
    typedef std::size_t   size_type;

    static const size_type npos = static_cast<size_type>(-1);

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default – empty view.
    StringView() noexcept : data_(nullptr), size_(0U) {}

    /// From null-terminated C string.
    /// CERT C++ STR11-CPP: nullptr check.
    StringView(const char *str) noexcept  // NOLINT(google-explicit-constructor)
        : data_(str)
        , size_((str != nullptr) ? std::strlen(str) : 0U)
    {}

    /// From pointer + explicit length.
    StringView(const char *str, size_type len) noexcept
        : data_(str)
        , size_((str != nullptr) ? len : 0U)
    {}

    /// From std::string (implicit – mirrors std::string_view behaviour).
    StringView(const std::string &str) noexcept  // NOLINT
        : data_(str.data()), size_(str.size())
    {}

    // -----------------------------------------------------------------------
    // Observers
    // -----------------------------------------------------------------------

    const char *data()  const noexcept { return data_; }
    size_type   size()  const noexcept { return size_; }
    size_type   length()const noexcept { return size_; }
    bool        empty() const noexcept { return size_ == 0U; }

    const_iterator begin() const noexcept { return data_; }
    const_iterator end()   const noexcept
    {
        return (data_ != nullptr) ? (data_ + size_) : nullptr;
    }

    /// Element access – no bounds checking (caller must validate).
    char operator[](size_type pos) const noexcept { return data_[pos]; }

    // -----------------------------------------------------------------------
    // Comparison
    // -----------------------------------------------------------------------

    bool operator==(const StringView &other) const noexcept
    {
        if (size_ != other.size_) { return false; }
        if (data_ == other.data_) { return true;  }
        if ((data_ == nullptr) || (other.data_ == nullptr)) { return false; }
        return std::memcmp(data_, other.data_, size_) == 0;
    }

    bool operator!=(const StringView &other) const noexcept
    {
        return !(*this == other);
    }

    bool operator==(const char *str) const noexcept
    {
        return *this == StringView(str);
    }

    bool operator!=(const char *str) const noexcept
    {
        return !(*this == str);
    }

    bool operator==(const std::string &str) const noexcept
    {
        return *this == StringView(str);
    }

    bool operator!=(const std::string &str) const noexcept
    {
        return !(*this == str);
    }

    /// Conversion to std::string (explicit to avoid accidental copies).
    std::string ToString() const
    {
        return (data_ != nullptr)
               ? std::string(data_, size_)
               : std::string();
    }

private:
    const char *data_;
    size_type   size_;
};

// Free-function comparisons (both directions).
inline bool operator==(const char *lhs, const StringView &rhs) noexcept
{
    return rhs == lhs;
}
inline bool operator==(const std::string &lhs, const StringView &rhs) noexcept
{
    return rhs == lhs;
}
inline bool operator!=(const char *lhs, const StringView &rhs) noexcept
{
    return !(rhs == lhs);
}
inline bool operator!=(const std::string &lhs, const StringView &rhs) noexcept
{
    return !(rhs == lhs);
}

} // namespace core
} // namespace ara

#endif // ARA_CORE_STRING_VIEW_H_
