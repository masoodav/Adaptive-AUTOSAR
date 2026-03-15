/**
 * @file logger.cpp
 * @brief ara::log::Logger and CreateLogger implementations – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00172]  class Logger
 *   [SWS_LOG_00260]  ~Logger()
 *   [SWS_LOG_00070]  IsEnabled()
 *   [SWS_LOG_00064-00069] LogFatal / LogError / LogWarn / LogInfo / LogDebug / LogVerbose
 *   [SWS_LOG_00131]  WithLevel()
 *   [SWS_LOG_00255]  SetThreshold()  [SWS_LOG_00254]
 *   [SWS_LOG_00021]  CreateLogger(StringView,StringView,LogLevel)
 *   [SWS_LOG_00263]  CreateLogger(StringView,StringView)  [SWS_LOG_00253]
 *   [SWS_LOG_00256]  CreateLogger(InstanceSpecifier)
 *   [SWS_LOG_00205]  RegisterConnectionStateHandler
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#include "./logger.h"
#include "./log_stream.h"
#include "./log_backend.h"

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// Private constructor [SWS_LOG_00006]
// ---------------------------------------------------------------------------

Logger::Logger(ara::core::StringView ctxId,
               ara::core::StringView ctxDescription,
               LogLevel              threshold) noexcept
    : ctxId_         (ctxId.data(), ctxId.size())
    , ctxDescription_(ctxDescription.data(), ctxDescription.size())
    , threshold_     (threshold)
{
    // Registration is implicit – Logger is stored in the LoggingFramework
    // map by GetOrCreateLogger. [SWS_LOG_00005]
}

// ---------------------------------------------------------------------------
// Move constructor – required so std::vector<Logger> can relocate elements.
// [SWS_LOG_00172] Logger class
// ---------------------------------------------------------------------------

Logger::Logger(Logger &&other) noexcept
    : ctxId_         (std::move(other.ctxId_))
    , ctxDescription_(std::move(other.ctxDescription_))
    , threshold_     (other.threshold_)
{
    // After move, the source Logger is in a valid but empty state.
    // The LoggingFramework singleton still holds the authoritative record;
    // this move is only used to allow std::vector<Logger> to store objects.
    other.threshold_ = LogLevel::kOff;
}


// ---------------------------------------------------------------------------
// [SWS_LOG_00260] Destructor
// ---------------------------------------------------------------------------

Logger::~Logger()
{
    // De-registration happens automatically when the LoggingFramework
    // destroys its owned Logger instances during shutdown. [SWS_LOG_00005]
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00070] IsEnabled  [SWS_LOG_00007]
// ---------------------------------------------------------------------------

bool Logger::IsEnabled(LogLevel logLevel) const noexcept
{
    // Severity ordering: smaller numeric value = higher severity.
    // kOff (0) means logging is completely suppressed.
    if (threshold_ == LogLevel::kOff) { return false; }
    if (logLevel  == LogLevel::kOff) { return false; }

    // MISRA C++:2023 Rule 15.0.1: explicit cast to underlying type.
    return static_cast<std::uint8_t>(logLevel)
           <= static_cast<std::uint8_t>(threshold_);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00255] SetThreshold  [SWS_LOG_00254]
// ---------------------------------------------------------------------------

void Logger::SetThreshold(LogLevel threshold) noexcept
{
    threshold_ = threshold;
}

// ---------------------------------------------------------------------------
// Log-level shorthand methods [SWS_LOG_00064..00069]
// LogStream private constructor is accessible via friendship.
// ---------------------------------------------------------------------------

LogStream Logger::LogFatal() const noexcept   // [SWS_LOG_00008]
{
    return LogStream(LogLevel::kFatal,   IsEnabled(LogLevel::kFatal));
}

LogStream Logger::LogError() const noexcept   // [SWS_LOG_00009]
{
    return LogStream(LogLevel::kError,   IsEnabled(LogLevel::kError));
}

LogStream Logger::LogWarn() const noexcept    // [SWS_LOG_00010]
{
    return LogStream(LogLevel::kWarn,    IsEnabled(LogLevel::kWarn));
}

LogStream Logger::LogInfo() const noexcept    // [SWS_LOG_00011]
{
    return LogStream(LogLevel::kInfo,    IsEnabled(LogLevel::kInfo));
}

LogStream Logger::LogDebug() const noexcept   // [SWS_LOG_00012]
{
    return LogStream(LogLevel::kDebug,   IsEnabled(LogLevel::kDebug));
}

LogStream Logger::LogVerbose() const noexcept // [SWS_LOG_00013]
{
    return LogStream(LogLevel::kVerbose, IsEnabled(LogLevel::kVerbose));
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00131] WithLevel  [SWS_LOG_00130]
// ---------------------------------------------------------------------------

LogStream Logger::WithLevel(LogLevel logLevel) const noexcept
{
    return LogStream(logLevel, IsEnabled(logLevel));
}

// ---------------------------------------------------------------------------
// Internal accessors
// ---------------------------------------------------------------------------

ara::core::StringView Logger::ContextId() const noexcept
{
    return ara::core::StringView(ctxId_);
}

ara::core::StringView Logger::ContextDescription() const noexcept
{
    return ara::core::StringView(ctxDescription_);
}

LogLevel Logger::Threshold() const noexcept
{
    return threshold_;
}

// ---------------------------------------------------------------------------
// CreateLogger factory functions
// ---------------------------------------------------------------------------

Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription,
                     LogLevel              ctxDefLogLevel) noexcept // [SWS_LOG_00021]
{
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, ctxDefLogLevel);
}

Logger &CreateLogger(ara::core::StringView ctxId,
                     ara::core::StringView ctxDescription) noexcept // [SWS_LOG_00263]
{
    // [SWS_LOG_00253]: use manifest level, fall back to kWarn.
    const LogLevel manifestLevel =
        internal::LoggingFramework::Instance().GetManifestLogLevel(ctxId);
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, manifestLevel);
}

Logger &CreateLogger(const ara::core::InstanceSpecifier &is) noexcept // [SWS_LOG_00256]
{
    const ara::core::StringView ctxId = is.ToString();
    const ara::core::StringView ctxDescription =
        ara::core::StringView("Created from InstanceSpecifier");
    const LogLevel manifestLevel =
        internal::LoggingFramework::Instance().GetManifestLogLevel(ctxId);
    return internal::LoggingFramework::Instance()
               .GetOrCreateLogger(ctxId, ctxDescription, manifestLevel);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00205] RegisterConnectionStateHandler
// ---------------------------------------------------------------------------

void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept
{
    internal::LoggingFramework::Instance()
        .RegisterConnectionStateHandler(std::move(callback));
}

} // namespace log
} // namespace ara
