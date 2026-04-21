#include "./logging_framework.h"

#include <deque>

namespace ara
{
namespace log
{

namespace
{
std::deque<LoggingFramework>& FrameworkStorage()
{
    static std::deque<LoggingFramework> frameworks;
    return frameworks;
}
}

// Constructor
LoggingFramework::LoggingFramework(
    const std::shared_ptr<sink::LogSink>& logSink,
    LogLevel logLevel)
    : mLogSink(logSink),
      mDefaultLogLevel(logLevel)
{
}

// Create logger (default level)
const Logger& LoggingFramework::CreateLogger(
    std::string ctxId,
    std::string ctxDescription)
{
    Logger logger =
        ara::log::CreateLogger(ctxId, ctxDescription, mDefaultLogLevel);

    mLoggers.push_back(std::move(logger));
    return mLoggers.back();
}

// Create logger (explicit level)
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

// Log dispatch
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

// Factory (console)
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

        // ✅ FIX: avoid emplace_back (private ctor issue)
        FrameworkStorage().push_back(LoggingFramework(logSink, logLevel));
        return &FrameworkStorage().back();
    }

    throw std::invalid_argument("Unsupported log mode.");
}

// Factory (file)
LoggingFramework* LoggingFramework::Create(
    std::string appId,
    std::string filePath,
    LogLevel logLevel,
    std::string appDescription)
{
    std::shared_ptr<sink::LogSink> logSink =
        std::make_shared<sink::FileLogSink>(appId, appDescription, filePath);

    // ✅ FIX: avoid emplace_back (private ctor issue)
    FrameworkStorage().push_back(LoggingFramework(logSink, logLevel));
    return &FrameworkStorage().back();
}

// Destructor
LoggingFramework::~LoggingFramework() noexcept = default;

} // namespace log
} // namespace ara
