/**
 * @file log_stream.h
 * @brief ara::log::LogStream and ara::log::Argument – C++14 compliant.
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
 *   [SWS_LOG_00040-00050]  arithmetic operator<<
 *   [SWS_LOG_00062]  operator<<(StringView)
 *   [SWS_LOG_00051]  operator<<(const char*)
 *   [SWS_LOG_00128]  operator<<(Span<const Byte>)
 *   [SWS_LOG_00203]  operator<<(Argument<T>)
 *   [SWS_LOG_00125]  operator<<(chrono::duration)  non-member template
 *   [SWS_LOG_00126]  operator<<(InstanceSpecifier)
 *   [SWS_LOG_00127]  operator<<(const void*)
 *   [SWS_LOG_00063]  operator<<(LogLevel)
 *   [SWS_LOG_00124]  operator<<(ErrorCode)
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_LOG_STREAM_H_
#define ARA_LOG_LOG_STREAM_H_

#include "common.h"
#include "format.h"
#include "./string_view.h"
#include "./span.h"
#include "../core/instance_specifier.h"
#include "../core/error_code.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>

// Forward declaration so LogSink can be named as a friend of LogStream.
namespace ara { namespace log { namespace sink { class LogSink; } } }

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00261] class Argument<T>
// ---------------------------------------------------------------------------

/**
 * @brief Wrapper holding a payload argument with optional name/unit/format.
 * Constructed via ara::log::Arg() factory only.
 * @tparam T  The argument payload type.
 */
template <typename T>
class Argument final
{
public:
    T             value;
    const char   *name;
    const char   *unit;
    Format        fmt;

    Argument(T v, const char *n, const char *u, Format f) noexcept
        : value(v), name(n), unit(u), fmt(f)
    {}
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00173] class LogStream
// ---------------------------------------------------------------------------

/**
 * @brief Represents a log message under construction.
 *
 * Obtained from Logger methods only; never constructed directly.
 * Thread-safety: NOT thread-safe.
 * Exception-safety: all member functions noexcept.  [SWS_LOG_00002]
 */
class LogStream final
{
public:
    // -----------------------------------------------------------------------
    // Special member functions
    // -----------------------------------------------------------------------
    /**
     * @brief Default constructor.
     *
     * Creates an active LogStream at LogLevel::kInfo with an empty buffer,
     * ready for operator<< use.
     *
     * Required by the project's log_sink.cpp which default-constructs:
     *   LogStream _result;
     * inside GetAppstamp() (line 16) and GetTimestamp() (line 32).
     */
    LogStream() noexcept;

    LogStream(const LogStream &)            = delete;  // [SWS_LOG_00174]
    LogStream(LogStream &&other)            noexcept;  // [SWS_LOG_00176]
    LogStream &operator=(const LogStream &) = delete;  // [SWS_LOG_00175]
    LogStream &operator=(LogStream &&)      = delete;  // [SWS_LOG_00177]
    ~LogStream()                            noexcept;  // [SWS_LOG_00262]

    // -----------------------------------------------------------------------
    // Stream control
    // -----------------------------------------------------------------------
    void       Flush()                                               noexcept; // [SWS_LOG_00039]
    LogStream &WithLocation(ara::core::StringView file, int line)   noexcept; // [SWS_LOG_00129]
    LogStream &WithTag(ara::core::StringView tag)                   noexcept; // [SWS_LOG_00132]

    // [SWS_LOG_00258] WithPrivacy – T must be integral or enum.
    template <typename T>
    LogStream &WithPrivacy(T value) noexcept
    {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value,
            "[SWS_LOG_00258] WithPrivacy: T must be integral or enum type.");
        privacy_    = static_cast<std::uint8_t>(
                          static_cast<std::uint64_t>(value) & 0xFFU);
        hasPrivacy_ = true;
        return *this;
    }

    // -----------------------------------------------------------------------
    // Arithmetic operator<< [SWS_LOG_00040..00050]
    // -----------------------------------------------------------------------
    LogStream &operator<<(bool          value) noexcept;
    LogStream &operator<<(std::uint8_t  value) noexcept;
    LogStream &operator<<(std::uint16_t value) noexcept;
    LogStream &operator<<(std::uint32_t value) noexcept;
    LogStream &operator<<(std::uint64_t value) noexcept;
    LogStream &operator<<(std::int8_t   value) noexcept;
    LogStream &operator<<(std::int16_t  value) noexcept;
    LogStream &operator<<(std::int32_t  value) noexcept;
    LogStream &operator<<(std::int64_t  value) noexcept;
    LogStream &operator<<(float         value) noexcept;
    LogStream &operator<<(double        value) noexcept;

    // -----------------------------------------------------------------------
    // String / raw-data operator<<
    // -----------------------------------------------------------------------
    LogStream &operator<<(ara::core::StringView                   value) noexcept; // [SWS_LOG_00062]
    LogStream &operator<<(const char *const                       value) noexcept; // [SWS_LOG_00051]
    LogStream &operator<<(ara::core::Span<const ara::core::Byte>  data)  noexcept; // [SWS_LOG_00128]

    // -----------------------------------------------------------------------
    // [SWS_LOG_00203] Argument<T>
    // -----------------------------------------------------------------------
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

    // -----------------------------------------------------------------------
    // Internal accessors
    // -----------------------------------------------------------------------
    LogLevel           Level()    const noexcept { return level_;    }
    const std::string &Buffer()   const noexcept { return buffer_;   }
    /// Return a copy of the payload as a std::string.
    /// Used by LogSink implementations to format the final log line.
    std::string           ToString()  const          { return buffer_; }
    bool               IsActive() const noexcept { return isActive_; }

private:
    // Private constructor – only Logger may instantiate.
    explicit LogStream(LogLevel level, bool enabled) noexcept;
    void     DoFlush() noexcept;

    // -----------------------------------------------------------------------
    // Member data – declared in construction order to avoid -Wreorder.
    // -----------------------------------------------------------------------
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
    // LogSink helpers (GetTimestamp, GetAppstamp) construct LogStream.
    friend class ::ara::log::sink::LogSink;
    // [FIX-E]: Allow LogStream-to-LogStream operator<< to access buffer_.
    friend LogStream &operator<<(LogStream &out,
                                 const LogStream &other) noexcept;
};

// ---------------------------------------------------------------------------
// Non-member operator<< – templates and declarations
// ---------------------------------------------------------------------------

/// [SWS_LOG_00125] std::chrono::duration
template <typename Rep, typename Period>
LogStream &operator<<(LogStream                                 &out,
                      const std::chrono::duration<Rep, Period> &value) noexcept
{
    out << static_cast<std::int64_t>(value.count());
    return out;
}

/// [SWS_LOG_00126]
LogStream &operator<<(LogStream &out,
                      const ara::core::InstanceSpecifier &value) noexcept;

/// [SWS_LOG_00127]
LogStream &operator<<(LogStream &out, const void *value) noexcept;

/// [SWS_LOG_00063]
LogStream &operator<<(LogStream &out, LogLevel value) noexcept;

/// [SWS_LOG_00124]
LogStream &operator<<(LogStream &out,
                      const ara::core::ErrorCode &ec) noexcept;

/// Append the payload of another LogStream into this one.
/// Used by LoggingFramework::Log() to merge context prefix with message.
LogStream &operator<<(LogStream &out, const LogStream &other) noexcept;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_STREAM_H_
