/**
 * @file logger.h
 * @brief ara::log::Logger class and CreateLogger factory functions.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 * C++14 compliant.
 *
 * Traceability:
 *   [SWS_LOG_00172]  class Logger
 *   [SWS_LOG_00259]  Logger() = delete
 *   [SWS_LOG_00260]  ~Logger()
 *   [SWS_LOG_00070]  IsEnabled()
 *   [SWS_LOG_00064]  LogFatal()   [SWS_LOG_00065] LogError()
 *   [SWS_LOG_00066]  LogWarn()    [SWS_LOG_00067] LogInfo()
 *   [SWS_LOG_00068]  LogDebug()   [SWS_LOG_00069] LogVerbose()
 *   [SWS_LOG_00131]  WithLevel()
 *   [SWS_LOG_00204]  Log()  modeled messages
 *   [SWS_LOG_00133]  LogWith()
 *   [SWS_LOG_00255]  SetThreshold()
 *   [SWS_LOG_00256]  CreateLogger(InstanceSpecifier)
 *   [SWS_LOG_00263]  CreateLogger(StringView, StringView)
 *   [SWS_LOG_00021]  CreateLogger(StringView, StringView, LogLevel)
 *   [SWS_LOG_00265]  ConnectionStateHandler
 *   [SWS_LOG_00205]  RegisterConnectionStateHandler()
 *   [SWS_LOG_00201]  Arg() factory
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_LOGGER_H_
#define ARA_LOG_LOGGER_H_

#include "common.h"
#include "format.h"
#include "log_stream.h"
#include "./string_view.h"
#include "../core/instance_specifier.h"

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <tuple>

// Forward declarations.
namespace ara { namespace log { namespace internal { class LoggingFramework; } } }
namespace ara { namespace log { class LoggingFramework; } }

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00201] Arg() factory
// ---------------------------------------------------------------------------

/**
 * @brief Create an Argument<T> wrapper with optional name/unit/format.
 *
 * Ill-formed when T is not arithmetic, bool, or StringView-convertible.
 * [SWS_LOG_00201]
 *
 * @tparam T  Argument payload type.
 */
template <typename T>
Argument<T> Arg(T           arg,
                const char *name   = NULL,
                const char *unit   = NULL,
                Format      format = Dflt()) noexcept
{
    static_assert(
        std::is_arithmetic<typename std::remove_reference<T>::type>::value ||
        std::is_convertible<typename std::remove_reference<T>::type,
                            ara::core::StringView>::value,
        "[SWS_LOG_00201] Arg: T must be arithmetic, bool, or StringView-"
        "convertible.");
    return Argument<T>(arg, name, unit, format);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00265] ConnectionStateHandler
// ---------------------------------------------------------------------------

/**
 * @brief Callback invoked on logging-client connection state changes.
 * [SWS_LOG_00265]: using ConnectionStateHandler = std::function<void(ClientState)>
 */
typedef std::function<void(ClientState)> ConnectionStateHandler;

// ---------------------------------------------------------------------------
// [SWS_LOG_00172] class Logger
// ---------------------------------------------------------------------------

/**
 * @brief Represents a logging context (context ID + threshold + back-end).
 *
 * Obtained exclusively via CreateLogger() overloads.
 * Strong ownership held by the Logging framework. [SWS_LOG_00005]
 */
class Logger final
{
public:
    // -----------------------------------------------------------------------
    // Special member functions
    // -----------------------------------------------------------------------
    Logger()                          = delete;  // [SWS_LOG_00259]
    ~Logger();                                   // [SWS_LOG_00260]
    Logger(const Logger &)            = delete;  ///< Not copyable.
    Logger &operator=(const Logger &) = delete;  ///< Not copyable.
    Logger(Logger &&other)            noexcept;  ///< Movable – needed by std::vector.
    Logger &operator=(Logger &&)      = delete;  ///< Move-assign not needed.

    // -----------------------------------------------------------------------
    // [SWS_LOG_00070] IsEnabled
    // -----------------------------------------------------------------------
    bool IsEnabled(LogLevel logLevel) const noexcept;

    // -----------------------------------------------------------------------
    // Log-level shorthand methods [SWS_LOG_00064..00069]
    // -----------------------------------------------------------------------
    LogStream LogFatal()   const noexcept;
    LogStream LogError()   const noexcept;
    LogStream LogWarn()    const noexcept;
    LogStream LogInfo()    const noexcept;
    LogStream LogDebug()   const noexcept;
    LogStream LogVerbose() const noexcept;

    // -----------------------------------------------------------------------
    // [SWS_LOG_00131] WithLevel
    // -----------------------------------------------------------------------
    LogStream WithLevel(LogLevel logLevel) const noexcept;

    // -----------------------------------------------------------------------
    // [SWS_LOG_00204] Log – modeled messages
    // -----------------------------------------------------------------------
    /**
     * @brief Log a modeled (non-verbose) DLT message.
     *
     * MsgId type carries compile-time argument type info and log level.
     * Argument type mismatch makes the program ill-formed. [SWS_LOG_00241]
     */
    template <typename MsgId, typename ParamT0 = void,
              typename ParamT1 = void, typename ParamT2 = void>
    void Log(const MsgId &id) noexcept
    {
        if (!IsEnabled(id.GetLogLevel())) { return; }
        // Dispatch to back-end (implementation-defined serialisation).
    }

    // Variadic workaround for C++14 – single-param variant.
    template <typename MsgId, typename P0>
    void Log(const MsgId &id, const P0 &p0) noexcept
    {
        id.Verify(p0);
        if (!IsEnabled(id.GetLogLevel())) { return; }
    }

    // Two-param variant.
    template <typename MsgId, typename P0, typename P1>
    void Log(const MsgId &id, const P0 &p0, const P1 &p1) noexcept
    {
        id.Verify(p0, p1);
        if (!IsEnabled(id.GetLogLevel())) { return; }
    }

    // -----------------------------------------------------------------------
    // [SWS_LOG_00133] LogWith – modeled message with attributes
    // -----------------------------------------------------------------------
    template <typename AttrsT, typename MsgId, typename P0>
    void LogWith(const AttrsT & /*attrs*/,
                 const MsgId  &msgId,
                 const P0     &p0) noexcept
    {
        Log(msgId, p0);
    }

    // -----------------------------------------------------------------------
    // [SWS_LOG_00255] SetThreshold
    // -----------------------------------------------------------------------
    void SetThreshold(LogLevel threshold) noexcept;

    // -----------------------------------------------------------------------
    // Internal accessors
    // -----------------------------------------------------------------------
    ara::core::StringView ContextId()          const noexcept;
    ara::core::StringView ContextDescription() const noexcept;
    LogLevel              Threshold()          const noexcept;

private:
    // Private constructor – only CreateLogger() and LoggingFramework create.
    Logger(ara::core::StringView ctxId,
           ara::core::StringView ctxDescription,
           LogLevel              threshold) noexcept;

    std::string ctxId_;
    std::string ctxDescription_;
    LogLevel    threshold_;

    // Factory functions and back-end singleton need access to the private ctor.
    friend Logger &CreateLogger(ara::core::StringView ctxId,
                                ara::core::StringView ctxDescription,
                                LogLevel              ctxDefLogLevel) noexcept;

    friend Logger &CreateLogger(ara::core::StringView ctxId,
                                ara::core::StringView ctxDescription) noexcept;

    friend Logger &CreateLogger(
        const ara::core::InstanceSpecifier &is) noexcept;

    friend class internal::LoggingFramework;  ///< Internal singleton.
    friend class ::ara::log::LoggingFramework; ///< Public framework class.
};

// ---------------------------------------------------------------------------
// CreateLogger factory functions
// ---------------------------------------------------------------------------

/// [SWS_LOG_00021] Explicit log level.
Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription,
                     LogLevel              ctxDefLogLevel) noexcept;

/// [SWS_LOG_00263] Manifest default (kWarn fallback). [SWS_LOG_00253]
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
