/**
 * @file logger.cpp
 * @brief Implementation of ara::log::Logger and CreateLogger factory functions.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
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
 *   [SWS_LOG_00255]  SetThreshold()
 *   [SWS_LOG_00254]  SetThreshold behaviour
 *   [SWS_LOG_00256]  CreateLogger(InstanceSpecifier)
 *   [SWS_LOG_00263]  CreateLogger(StringView, StringView)
 *   [SWS_LOG_00021]  CreateLogger(StringView, StringView, LogLevel)
 *   [SWS_LOG_00005]  Logger created/stored internally; reference returned
 *   [SWS_LOG_00006]  Logger created with ctxId, description, threshold
 *   [SWS_LOG_00253]  Default log level from manifest (kWarn fallback)
 *   [SWS_LOG_00007]  IsEnabled for pre-filtering
 *   [SWS_LOG_00008]  LogFatal  → LogStream
 *   [SWS_LOG_00009]  LogError  → LogStream
 *   [SWS_LOG_00010]  LogWarn   → LogStream
 *   [SWS_LOG_00011]  LogInfo   → LogStream
 *   [SWS_LOG_00012]  LogDebug  → LogStream
 *   [SWS_LOG_00013]  LogVerbose→ LogStream
 *   [SWS_LOG_00130]  WithLevel → LogStream
 *   [SWS_LOG_00205]  RegisterConnectionStateHandler
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 6.4.1  – explicit underlying types for enums
 *   - MISRA C++:2023 Rule 15.0.1 – no implicit integral conversions
 *   - CERT C++ OOP54-CPP         – copy/move suppressed for owned objects
 *   - CERT C++ CON54-CPP         – thread-safe via LoggingFramework mutex
 *   - CWE-362                    – no unsynchronised access to shared state
 *   - ISO/SAE 21434              – no exceptions, no UB in control path
 */

#include "ara/log/logger.h"
#include "ara/log/log_stream.h"
#include "ara/log/internal/log_backend.h"

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// Logger private constructor
// [SWS_LOG_00006]
// ---------------------------------------------------------------------------

Logger::Logger(ara::core::StringView ctxId,
               ara::core::StringView ctxDescription,
               LogLevel              threshold) noexcept
    : ctxId_         {ctxId.data(), ctxId.size()}
    , ctxDescription_{ctxDescription.data(), ctxDescription.size()}
    , threshold_     {threshold}
{
    // Registration against the logging back-end is implicit: the Logger
    // is stored in the LoggingFramework's internal map via GetOrCreateLogger.
    // [SWS_LOG_00005]: "Logger instance created internally inside the Logging
    // framework and returned as reference to the using application."
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00260] Destructor
// ---------------------------------------------------------------------------

Logger::~Logger()
{
    // De-registration from the logging back-end is handled by the
    // LoggingFramework when it destroys owned Logger instances during
    // shutdown. No explicit action required here per the spec note:
    // "automatically deregistered during process shutdown phase."
    // [SWS_LOG_00005]
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00070] IsEnabled
// [SWS_LOG_00007]
// ---------------------------------------------------------------------------

bool Logger::IsEnabled(LogLevel logLevel) const noexcept
{
    // A message is enabled when its level is:
    //   - Not kOff (special "no logging" sentinel).
    //   - At least as severe as the configured threshold.
    //
    // Severity ordering: kFatal(1) > kError(2) > kWarn(3) > kInfo(4)
    //                    > kDebug(5) > kVerbose(6)
    // i.e. lower numeric value == higher severity.
    // threshold_ == kOff means all logging is suppressed.
    if (threshold_ == LogLevel::kOff)
    {
        return false;
    }
    if (logLevel == LogLevel::kOff)
    {
        return false;
    }
    // MISRA C++:2023 Rule 15.0.1: explicit cast to underlying type for
    // comparison – prevents implicit conversions.
    return static_cast<std::uint8_t>(logLevel)
           <= static_cast<std::uint8_t>(threshold_);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00255] SetThreshold
// [SWS_LOG_00254]
// ---------------------------------------------------------------------------

void Logger::SetThreshold(LogLevel threshold) noexcept
{
    threshold_ = threshold;
}

// ---------------------------------------------------------------------------
// Internal helper: create a LogStream for a given level.
// [SWS_LOG_00008..00013], [SWS_LOG_00130]
// ---------------------------------------------------------------------------

// Note: LogStream's private constructor is accessed via friendship.
// The constructor is: LogStream(LogLevel level, bool enabled) noexcept.

/// [SWS_LOG_00064] LogFatal – [SWS_LOG_00008]
LogStream Logger::LogFatal() const noexcept
{
    return LogStream{LogLevel::kFatal, IsEnabled(LogLevel::kFatal)};
}

/// [SWS_LOG_00065] LogError – [SWS_LOG_00009]
LogStream Logger::LogError() const noexcept
{
    return LogStream{LogLevel::kError, IsEnabled(LogLevel::kError)};
}

/// [SWS_LOG_00066] LogWarn – [SWS_LOG_00010]
LogStream Logger::LogWarn() const noexcept
{
    return LogStream{LogLevel::kWarn, IsEnabled(LogLevel::kWarn)};
}

/// [SWS_LOG_00067] LogInfo – [SWS_LOG_00011]
LogStream Logger::LogInfo() const noexcept
{
    return LogStream{LogLevel::kInfo, IsEnabled(LogLevel::kInfo)};
}

/// [SWS_LOG_00068] LogDebug – [SWS_LOG_00012]
LogStream Logger::LogDebug() const noexcept
{
    return LogStream{LogLevel::kDebug, IsEnabled(LogLevel::kDebug)};
}

/// [SWS_LOG_00069] LogVerbose – [SWS_LOG_00013]
LogStream Logger::LogVerbose() const noexcept
{
    return LogStream{LogLevel::kVerbose, IsEnabled(LogLevel::kVerbose)};
}

/// [SWS_LOG_00131] WithLevel – [SWS_LOG_00130]
LogStream Logger::WithLevel(LogLevel logLevel) const noexcept
{
    return LogStream{logLevel, IsEnabled(logLevel)};
}

// ---------------------------------------------------------------------------
// Internal accessors
// ---------------------------------------------------------------------------

ara::core::StringView Logger::ContextId() const noexcept
{
    return ara::core::StringView{ctxId_};
}

ara::core::StringView Logger::ContextDescription() const noexcept
{
    return ara::core::StringView{ctxDescription_};
}

LogLevel Logger::Threshold() const noexcept
{
    return threshold_;
}

// ---------------------------------------------------------------------------
// CreateLogger factory functions
// ---------------------------------------------------------------------------

/**
 * [SWS_LOG_00021]
 * Create or retrieve a Logger with an explicit log-level threshold.
 */
Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription,
                     LogLevel              ctxDefLogLevel) noexcept
{
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, ctxDefLogLevel);
}

/**
 * [SWS_LOG_00263], [SWS_LOG_00253]
 * Create or retrieve a Logger using the manifest default log level.
 * Falls back to kWarn if no manifest entry is found.
 */
Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription) noexcept
{
    const LogLevel manifestLevel{
        internal::LoggingFramework::Instance().GetManifestLogLevel(ctxId)};
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, manifestLevel);
}

/**
 * [SWS_LOG_00256]
 * Create or retrieve a Logger from an InstanceSpecifier.
 *
 * The InstanceSpecifier path is used as the context ID, and the manifest
 * default log level is applied. In a full implementation, the manifest would
 * be consulted for context ID, description, and log level.
 *
 * Violations reported (framework-internally) when:
 *   - InstanceSpecifier cannot be resolved in the model.
 *   - Port interface type mismatch.
 *   - No matching process mapping.
 *   - InstanceSpecifier already in use.
 */
Logger &CreateLogger(const ara::core::InstanceSpecifier &is) noexcept
{
    // Use the InstanceSpecifier path as the context ID.
    // A real implementation would resolve this via the manifest.
    const ara::core::StringView ctxId{is.ToString()};
    const ara::core::StringView ctxDescription{"Created from InstanceSpecifier"};
    const LogLevel manifestLevel{
        internal::LoggingFramework::Instance().GetManifestLogLevel(ctxId)};
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, manifestLevel);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00205] RegisterConnectionStateHandler
// ---------------------------------------------------------------------------

void RegisterConnectionStateHandler(
    ConnectionStateHandler callback) noexcept
{
    internal::LoggingFramework::Instance()
        .RegisterConnectionStateHandler(std::move(callback));
}

} // namespace log
} // namespace ara
