/**
 * @file logger.h
 * @brief ara::log::Logger class and CreateLogger factory functions.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00172]  class Logger
 *   [SWS_LOG_00259]  Logger() = delete
 *   [SWS_LOG_00260]  ~Logger()
 *   [SWS_LOG_00070]  IsEnabled()
 *   [SWS_LOG_00064]  LogFatal()
 *   [SWS_LOG_00065]  LogError()
 *   [SWS_LOG_00066]  LogWarn()
 *   [SWS_LOG_00067]  LogInfo()
 *   [SWS_LOG_00068]  LogDebug()
 *   [SWS_LOG_00069]  LogVerbose()
 *   [SWS_LOG_00131]  WithLevel()
 *   [SWS_LOG_00204]  Log() modeled messages
 *   [SWS_LOG_00133]  LogWith()
 *   [SWS_LOG_00255]  SetThreshold()
 *   [SWS_LOG_00256]  CreateLogger(InstanceSpecifier)
 *   [SWS_LOG_00263]  CreateLogger(StringView, StringView)
 *   [SWS_LOG_00021]  CreateLogger(StringView, StringView, LogLevel)
 *   [SWS_LOG_00265]  ConnectionStateHandler
 *   [SWS_LOG_00205]  RegisterConnectionStateHandler()
 *   [SWS_LOG_00201]  Arg() factory
 *   Format helpers are in ara/log/format.h
 *
 * Coding standards: MISRA C++:2023 | CERT C++ | CWE-safe | ISO/SAE 21434
 */

#ifndef ARA_LOG_LOGGER_H_
#define ARA_LOG_LOGGER_H_

#include "ara/log/common.h"
#include "ara/log/format.h"
#include "ara/log/log_stream.h"
#include "ara/core/string_view.h"
#include "ara/core/instance_specifier.h"

// Forward declaration of internal back-end (full definition in log_backend.h)
namespace ara { namespace log { namespace internal { class LoggingFramework; } } }

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00201] Arg() factory
// ---------------------------------------------------------------------------

/**
 * @brief Create an Argument<T> wrapper with optional attributes.
 *
 * Calling this is ill-formed if T is not arithmetic, bool, or convertible
 * to StringView/Span<const Byte>. [SWS_LOG_00201]
 */
template <typename T>
Argument<T> Arg(T         arg,
                const char *name   = nullptr,
                const char *unit   = nullptr,
                Format      format = Dflt()) noexcept
{
    static_assert(
        std::is_arithmetic<std::remove_reference_t<T>>::value ||
        std::is_convertible<std::remove_reference_t<T>,
                            ara::core::StringView>::value,
        "[SWS_LOG_00201] Arg: T must be arithmetic, bool, or StringView-convertible.");

    return Argument<T>{std::move(arg), name, unit, format};
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00265] ConnectionStateHandler
// ---------------------------------------------------------------------------

/**
 * @brief Callback type for remote-client connection state changes.
 */
using ConnectionStateHandler = std::function<void(ClientState)>;

// ---------------------------------------------------------------------------
// [SWS_LOG_00172] class Logger
// ---------------------------------------------------------------------------

/**
 * @brief Represents a logging context (context ID + threshold + back-end ref).
 *
 * Obtained exclusively via CreateLogger() overloads.
 * Owned by the Logging framework. [SWS_LOG_00005]
 */
class Logger final
{
public:
    Logger()                          = delete;   // [SWS_LOG_00259]
    ~Logger();                                    // [SWS_LOG_00260]
    Logger(const Logger &)            = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&)                 = delete;
    Logger &operator=(Logger &&)      = delete;

    // [SWS_LOG_00070]
    bool IsEnabled(LogLevel logLevel) const noexcept;

    // [SWS_LOG_00064..00069]
    LogStream LogFatal()   const noexcept;
    LogStream LogError()   const noexcept;
    LogStream LogWarn()    const noexcept;
    LogStream LogInfo()    const noexcept;
    LogStream LogDebug()   const noexcept;
    LogStream LogVerbose() const noexcept;

    // [SWS_LOG_00131]
    LogStream WithLevel(LogLevel logLevel) const noexcept;

    // [SWS_LOG_00204] Modeled message API
    template <typename MsgId, typename... Params>
    void Log(const MsgId &id, const Params &...args) noexcept
    {
        // [SWS_LOG_00241]: compile-time type check via MsgId::Verify.
        id.Verify(args...);
        if (!IsEnabled(id.GetLogLevel())) { return; }
        // Dispatch to back-end is implementation-defined.
    }

    // [SWS_LOG_00133] Modeled message with attributes
    template <typename... Attrs, typename MsgId, typename... Params>
    void LogWith(const std::tuple<Attrs...> &attrs,
                 const MsgId               &msgId,
                 const Params &...          params) noexcept
    {
        (void)attrs;
        Log(msgId, params...);
    }

    // [SWS_LOG_00255]
    void SetThreshold(LogLevel threshold) noexcept;

    // Internal accessors used by framework
    ara::core::StringView ContextId()          const noexcept;
    ara::core::StringView ContextDescription() const noexcept;
    LogLevel              Threshold()          const noexcept;

private:
    // Private constructor – CreateLogger() and LoggingFramework are friends.
    Logger(ara::core::StringView ctxId,
           ara::core::StringView ctxDescription,
           LogLevel              threshold) noexcept;

    std::string ctxId_;
    std::string ctxDescription_;
    LogLevel    threshold_;

    // Grant factory functions access to private constructor.
    friend Logger &CreateLogger(ara::core::StringView ctxId,
                                ara::core::StringView ctxDescription,
                                LogLevel              ctxDefLogLevel) noexcept;

    friend Logger &CreateLogger(ara::core::StringView ctxId,
                                ara::core::StringView ctxDescription) noexcept;

    friend Logger &CreateLogger(
        const ara::core::InstanceSpecifier &is) noexcept;

    // Grant LoggingFramework access to private constructor for emergency logger.
    friend class internal::LoggingFramework; // forward-declared below
};

// ---------------------------------------------------------------------------
// CreateLogger factory functions
// ---------------------------------------------------------------------------

/// [SWS_LOG_00021] Explicit log level.
Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription,
                     LogLevel              ctxDefLogLevel) noexcept;

/// [SWS_LOG_00263] Manifest default log level (kWarn fallback). [SWS_LOG_00253]
Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription) noexcept;

/// [SWS_LOG_00256] From InstanceSpecifier.
Logger &CreateLogger(const ara::core::InstanceSpecifier &is) noexcept;

// ---------------------------------------------------------------------------
// [SWS_LOG_00205] RegisterConnectionStateHandler
// ---------------------------------------------------------------------------
void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOGGER_H_
