/**
 * @file log_stream.h
 * @brief ara::log::LogStream and ara::log::Argument declarations.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00173]  class LogStream
 *   [SWS_LOG_00261]  class Argument<T>
 *   [SWS_LOG_00174]  Copy constructor deleted
 *   [SWS_LOG_00176]  Move constructor
 *   [SWS_LOG_00175]  Copy assignment deleted
 *   [SWS_LOG_00177]  Move assignment deleted
 *   [SWS_LOG_00262]  Destructor
 *   [SWS_LOG_00039]  Flush()
 *   [SWS_LOG_00129]  WithLocation()
 *   [SWS_LOG_00132]  WithTag()
 *   [SWS_LOG_00258]  WithPrivacy()
 *   [SWS_LOG_00040-00050] arithmetic operator<<
 *   [SWS_LOG_00062]  operator<<(StringView)
 *   [SWS_LOG_00051]  operator<<(const char*)
 *   [SWS_LOG_00128]  operator<<(Span<const Byte>)
 *   [SWS_LOG_00203]  operator<<(Argument<T>)
 *   [SWS_LOG_00125]  operator<<(chrono::duration)
 *   [SWS_LOG_00126]  operator<<(InstanceSpecifier)
 *   [SWS_LOG_00127]  operator<<(const void*)
 *   [SWS_LOG_00063]  operator<<(LogLevel)
 *   [SWS_LOG_00124]  operator<<(ErrorCode)
 *
 * Coding standards: MISRA C++:2023 | CERT C++ | CWE-safe | ISO/SAE 21434
 */

#ifndef ARA_LOG_LOG_STREAM_H_
#define ARA_LOG_LOG_STREAM_H_

#include "ara/log/common.h"
#include "ara/log/format.h"
#include "ara/core/string_view.h"
#include "ara/core/span.h"
#include "ara/core/instance_specifier.h"
#include "ara/core/error_code.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00261] class Argument<T>
// ---------------------------------------------------------------------------

/**
 * @brief Wrapper holding a payload argument and its attributes.
 * @tparam T  The argument payload type.
 */
template <typename T>
class Argument final
{
public:
    T             value;
    const char   *name {nullptr};
    const char   *unit {nullptr};
    Format        fmt;

    Argument(T v, const char *n, const char *u, Format f) noexcept
        : value{std::move(v)}, name{n}, unit{u}, fmt{f}
    {}
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00173] class LogStream
// ---------------------------------------------------------------------------

/**
 * @brief Represents a log message under construction.
 * Thread-safety: NOT thread-safe. Exception-safety: all methods noexcept.
 */
class LogStream final
{
public:
    LogStream(const LogStream &)            = delete;  // [SWS_LOG_00174]
    LogStream(LogStream &&other)            noexcept;  // [SWS_LOG_00176]
    LogStream &operator=(const LogStream &) = delete;  // [SWS_LOG_00175]
    LogStream &operator=(LogStream &&)      = delete;  // [SWS_LOG_00177]
    ~LogStream()                            noexcept;  // [SWS_LOG_00262]

    void      Flush()                                                   noexcept; // [SWS_LOG_00039]
    LogStream &WithLocation(ara::core::StringView file, int line)       noexcept; // [SWS_LOG_00129]
    LogStream &WithTag(ara::core::StringView tag)                       noexcept; // [SWS_LOG_00132]

    template <typename T>
    LogStream &WithPrivacy(T value) noexcept  // [SWS_LOG_00258]
    {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value,
            "[SWS_LOG_00258] T must be integral or enum.");
        privacy_    = static_cast<std::uint8_t>(
                          static_cast<std::uint64_t>(value) & 0xFFU);
        hasPrivacy_ = true;
        return *this;
    }

    // Arithmetic overloads [SWS_LOG_00040..00050]
    LogStream &operator<<(bool           value) noexcept;
    LogStream &operator<<(std::uint8_t   value) noexcept;
    LogStream &operator<<(std::uint16_t  value) noexcept;
    LogStream &operator<<(std::uint32_t  value) noexcept;
    LogStream &operator<<(std::uint64_t  value) noexcept;
    LogStream &operator<<(std::int8_t    value) noexcept;
    LogStream &operator<<(std::int16_t   value) noexcept;
    LogStream &operator<<(std::int32_t   value) noexcept;
    LogStream &operator<<(std::int64_t   value) noexcept;
    LogStream &operator<<(float          value) noexcept;
    LogStream &operator<<(double         value) noexcept;

    // String / raw-data
    LogStream &operator<<(ara::core::StringView                    value) noexcept;
    LogStream &operator<<(const char *const                        value) noexcept;
    LogStream &operator<<(ara::core::Span<const ara::core::Byte>   data)  noexcept;

    // [SWS_LOG_00203] Argument<T>
    template <typename T>
    LogStream &operator<<(const Argument<T> &arg) noexcept
    {
        *this << arg.value;
        try
        {
            if (arg.name != nullptr)
            {
                metaBuffer_.append("[name=");
                metaBuffer_.append(arg.name);
                metaBuffer_.append("] ");
            }
            if (arg.unit != nullptr)
            {
                metaBuffer_.append("[unit=");
                metaBuffer_.append(arg.unit);
                metaBuffer_.append("] ");
            }
        }
        catch (...) {}
        return *this;
    }

    // Internal accessors
    LogLevel           Level()    const noexcept { return level_;    }
    const std::string &Buffer()   const noexcept { return buffer_;   }
    bool               IsActive() const noexcept { return isActive_; }

private:
    explicit LogStream(LogLevel level, bool enabled) noexcept;
    void     DoFlush() noexcept;

    LogLevel     level_;
    bool         isActive_;
    bool         isFlushed_;
    bool         hasPrivacy_;
    std::uint8_t privacy_;
    int          locationLine_;

    std::string  buffer_;
    std::string  metaBuffer_;
    std::string  locationFile_;
    std::string  tag_;

    friend class Logger;
};

// ---------------------------------------------------------------------------
// Non-member operator<< declarations
// ---------------------------------------------------------------------------

template <typename Rep, typename Period>
LogStream &operator<<(LogStream &out,
                      const std::chrono::duration<Rep, Period> &value) noexcept
{
    out << static_cast<std::int64_t>(value.count());
    return out;
}

LogStream &operator<<(LogStream &out,
                      const ara::core::InstanceSpecifier &value) noexcept; // [SWS_LOG_00126]
LogStream &operator<<(LogStream &out, const void *value) noexcept;          // [SWS_LOG_00127]
LogStream &operator<<(LogStream &out, LogLevel value) noexcept;             // [SWS_LOG_00063]
LogStream &operator<<(LogStream &out,
                      const ara::core::ErrorCode &ec) noexcept;             // [SWS_LOG_00124]

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_STREAM_H_
