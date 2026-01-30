#include "./logging_framework.h"

namespace ara
{
    namespace log
    {
        LoggingFramework::LoggingFramework(
            sink::LogSink *logSink,
            LogLevel logLevel) : mLogSink{logSink},
                                 mDefaultLogLevel{logLevel}
        {
        }

        const Logger &LoggingFramework::CreateLogger(
            std::string ctxId,
            std::string ctxDescription)
        {
            // FIX: Call global factory which returns a reference to a managed instance
            const Logger& loggerRef = 
                ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), mDefaultLogLevel);
            
            // FIX: Store pointer to the managed instance
            mLoggers.push_back(&loggerRef);
            
            return loggerRef;
        }

        const Logger &LoggingFramework::CreateLogger(
            std::string ctxId,
            std::string ctxDescription,
            LogLevel ctxDefLogLevel)
        {
            // FIX: Call global factory
            const Logger& loggerRef =
                ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), ctxDefLogLevel);
            
            // FIX: Store pointer
            mLoggers.push_back(&loggerRef);
            
            return loggerRef;
        }

        void LoggingFramework::Log(
            const Logger &logger,
            LogLevel logLevel,
            const LogStream &logStream)
        {
            bool _isLevelEnabled = logger.IsEnabled(logLevel);

            if (_isLevelEnabled)
            {
                LogStream _logStreamContext = logger.WithLevel(logLevel);
                // FIX: Pipe content from one stream to another (requires operator<< overload in log_stream.h)
                _logStreamContext << logStream;
                mLogSink->Log(_logStreamContext);
            }
        }

        LoggingFramework *LoggingFramework::Create(
            std::string appId,
            LogMode logMode,
            LogLevel logLevel,
            std::string appDescription)
        {
            if (logMode == LogMode::kFile)
            {
                throw std::invalid_argument(
                    "File logging mode is not feasible within this constructor override.");
            }

            if (logMode == LogMode::kConsole)
            {
                sink::LogSink *_logSink =
                    new sink::ConsoleLogSink(appId, appDescription);
                LoggingFramework *_result =
                    new LoggingFramework(_logSink, logLevel);

                return _result;
            }
            else
            {
                throw std::invalid_argument(
                    "The log mode is not currently supported.");
            }
        }

        LoggingFramework *LoggingFramework::Create(
            std::string appId,
            std::string filePath,
            LogLevel logLevel,
            std::string appDescription)
        {
            sink::LogSink *_logSink =
                new sink::FileLogSink(filePath, appId, appDescription);
            LoggingFramework *_result =
                new LoggingFramework(_logSink, logLevel);

            return _result;
        }

        LoggingFramework::~LoggingFramework() noexcept
        {
            if (mLogSink) {
                delete mLogSink;
            }
        }
    }
}