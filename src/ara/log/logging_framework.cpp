#include "./logging_framework.h"

namespace ara
{
namespace log
{

LoggingFramework::LoggingFramework(
    const std::shared_ptr<sink::LogSink>& logSink,
    LogLevel logLevel)
    : mLogSink(logSink),
      mDefaultLogLevel(logLevel)
{
}

const Logger& LoggingFramework::CreateLogger(
    std::string ctxId,
    std::string ctxDescription)
{
    Logger logger =
        ara::log::CreateLogger(ctxId, ctxDescription, mDefaultLogLevel);

    mLoggers.push_back(std::move(logger));
    return mLoggers.back();
}

const Logger& LoggingFramework::CreateLogger(
    std::string ctxId,
    std::string ctxDescription,
    LogLevel ctxDefLogLevel)
{
    Logger logger =
        ara::log::CreateLogger(ctxId, ctxDescription, ctxDefLogLevel);

    mLoggers.push_back(std::move(logger));
    return mLoggers.back();
}

void LoggingFramework::Log(
    const Logger& logger,
    LogLevel logLevel,
    const LogStream& logStream)
{
    if (logger.IsEnabled(logLevel))
    {
        mLogSink->Log(logStream);
    }
}

LoggingFramework* LoggingFramework::Create(
    std::string appId,
    LogMode logMode,
    LogLevel logLevel,
    std::string appDescription)
{
    if (logMode == LogMode::kFile)
    {
        throw std::invalid_argument(
            "File logging mode is not supported in this overload.");
    }

    if (logMode == LogMode::kConsole)
    {
        std::shared_ptr<sink::LogSink> logSink =
            std::make_shared<sink::ConsoleLogSink>(appId, appDescription);
        return new LoggingFramework(logSink, logLevel);
    }

    throw std::invalid_argument("Unsupported log mode.");
}

LoggingFramework* LoggingFramework::Create(
    std::string appId,
    std::string filePath,
    LogLevel logLevel,
    std::string appDescription)
{
    std::shared_ptr<sink::LogSink> logSink =
        std::make_shared<sink::FileLogSink>(appId, appDescription, filePath);
    return new LoggingFramework(logSink, logLevel);
}

LoggingFramework::~LoggingFramework() noexcept = default;

}  // namespace log
}  // namespace ara
