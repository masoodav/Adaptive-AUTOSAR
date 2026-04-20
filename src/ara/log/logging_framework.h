#ifndef LOGGING_FRAMEWORK_H
#define LOGGING_FRAMEWORK_H

#include <memory>
#include <stdexcept>
#include <vector>

#include "./logger.h"
#include "./sink/log_sink.h"
#include "./sink/console_log_sink.h"
#include "./sink/file_log_sink.h"

namespace ara
{
namespace log
{

/// @brief Logging framework which links loggers to a log sink
class LoggingFramework
{
private:
    std::shared_ptr<sink::LogSink> mLogSink;
    LogLevel mDefaultLogLevel;
    std::vector<Logger> mLoggers;

    // Private constructor (factory enforced)
    LoggingFramework(const std::shared_ptr<sink::LogSink>& logSink, LogLevel logLevel);

public:
    LoggingFramework() = delete;

    // ✅ REQUIRED for container storage
    LoggingFramework(LoggingFramework&&) = default;
    LoggingFramework& operator=(LoggingFramework&&) = default;

    // Optional safety (prevents accidental copies)
    LoggingFramework(const LoggingFramework&) = delete;
    LoggingFramework& operator=(const LoggingFramework&) = delete;

    ~LoggingFramework() noexcept;

    /// @brief Create a logger
    const Logger& CreateLogger(
        std::string ctxId,
        std::string ctxDescription);

    /// @brief Create a logger with explicit level
    const Logger& CreateLogger(
        std::string ctxId,
        std::string ctxDescription,
        LogLevel ctxDefLogLevel);

    /// @brief Log a stream to the determined sink
    void Log(
        const Logger& logger,
        LogLevel logLevel,
        const LogStream& logStream);

    /// @brief Logging framework factory (console)
    static LoggingFramework* Create(
        std::string appId,
        LogMode logMode,
        LogLevel logLevel = LogLevel::kWarn,
        std::string appDescription = "");

    /// @brief Logging framework factory (file)
    static LoggingFramework* Create(
        std::string appId,
        std::string filePath,
        LogLevel logLevel = LogLevel::kWarn,
        std::string appDescription = "");
};

} // namespace log
} // namespace ara

#endif